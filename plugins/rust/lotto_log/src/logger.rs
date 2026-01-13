use log::LevelFilter;
use lotto_sys as raw;
use regex::Regex;
use std::ffi::CString;
use std::fs::{File, OpenOptions};
use std::io::{BufWriter, Write};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Mutex;

fn init_full(level: LevelFilter, redirect: Redirect) {
    LEVEL.store(level as usize, Ordering::Relaxed);

    // Bypass the default logging level check if we want trace
    // messages.
    log::set_max_level(if redirect.always_discard() {
        level
    } else {
        LevelFilter::Trace
    });

    {
        let mut g = TRACE_REDIRECT.try_lock().expect("single thread");
        *g = redirect;
    }
    log::set_logger(&LOGGER).expect("lotto_log init");
}

/// Default initialization of the Lotto logging system.
///
/// The log level is controlled by envvar `LOTTO_LOGGER_LEVEL`.
///
/// The behavior of `trace!` is controlled by `LOTTO_RUST_TRACE`.
pub fn init() {
    let level = std::env::var("LOTTO_LOGGER_LEVEL");
    let filter = match level.ok().as_deref() {
        Some("") | Some("silent") => LevelFilter::Off,
        Some("error") => LevelFilter::Error,
        Some("info") => LevelFilter::Info,
        Some("debug") => LevelFilter::Debug,
        Some("trace") => LevelFilter::Trace,
        Some(_) => LevelFilter::Error,
        None => LevelFilter::Info,
    };

    let var = std::env::var("LOTTO_RUST_TRACE").unwrap_or("discard".to_string());
    let redirect = Redirect::parse(&var);

    init_full(filter, redirect);
}

#[derive(Debug)]
enum RedirectKind {
    Discard,
    Stderr,
    Debug,
    File(BufWriter<File>),
}

impl RedirectKind {
    fn flush(&mut self) {
        if let Self::File(writer) = self {
            let _ = writer.flush();
        }
    }

    fn output(&mut self, record: &log::Record) {
        match self {
            Self::Discard => {}
            Self::Stderr => eprintln!("{}", format_message(record)),
            Self::Debug => log::debug!(target: "", "{}", format_message(record)),
            Self::File(writer) => {
                let mut buf = format_message(record);
                buf += "\n";
                let _ = writer.write_all(buf.as_bytes());
            }
        }
    }

    fn parse(input: &str) -> RedirectKind {
        match input {
            "debug" => Self::Debug,
            "stderr" => Self::Stderr,
            "discard" => Self::Discard,
            path => {
                match OpenOptions::new()
                    .write(true)
                    .truncate(true)
                    .create(true)
                    .open(path)
                {
                    Ok(file) => Self::File(BufWriter::new(file)),
                    Err(err) => {
                        eprintln!("Cannot open {} for writing: {}", path, err);
                        Self::Debug
                    }
                }
            }
        }
    }
}

#[derive(Debug)]
struct ConditionalRedirect {
    regex: Option<Regex>,
    kind: RedirectKind,
}

impl ConditionalRedirect {
    fn output(&mut self, record: &log::Record) -> bool {
        if self.regex.is_none() || self.regex.as_ref().unwrap().is_match(record.target()) {
            self.kind.output(record);
            self.flush();
            true
        } else {
            false
        }
    }

    fn flush(&mut self) {
        self.kind.flush()
    }

    fn always_discard(&self) -> bool {
        self.regex.is_none() && matches!(self.kind, RedirectKind::Discard)
    }

    fn parse(input: &str) -> ConditionalRedirect {
        let parts: Vec<_> = input.split("=").collect();
        match &parts[..] {
            [ref kind] => ConditionalRedirect {
                regex: None,
                kind: RedirectKind::parse(kind),
            },
            [ref regex, ref kind] => ConditionalRedirect {
                regex: Some(Regex::new(regex).expect("Invalid regex for LOTTO_RUST_TRACE")),
                kind: RedirectKind::parse(kind),
            },
            _ => {
                panic!("Invalid LOTTO_RUST_TRACE");
            }
        }
    }
}

#[derive(Debug)]
struct Redirect(Vec<ConditionalRedirect>);

impl Redirect {
    fn output(&mut self, record: &log::Record) {
        for r in &mut self.0 {
            if r.output(record) {
                return;
            }
        }
    }

    fn flush(&mut self) {
        for r in &mut self.0 {
            r.flush();
        }
    }

    fn always_discard(&self) -> bool {
        self.0.iter().all(|cr| cr.always_discard())
    }

    fn parse(input: &str) -> Redirect {
        Redirect(
            input
                .split(",")
                .map(|part| ConditionalRedirect::parse(part))
                .collect(),
        )
    }
}

pub struct LottoLogger;

static LOGGER: LottoLogger = LottoLogger;
static TRACE_REDIRECT: Mutex<Redirect> = Mutex::new(Redirect(Vec::new()));
static LEVEL: AtomicUsize = AtomicUsize::new(0);

fn format_message(record: &log::Record) -> String {
    if record.target() == "" {
        format!("{}", record.args())
    } else {
        format!("[{:>21}] {}", record.target(), record.args())
    }
}

fn format_message_escape_cstr(record: &log::Record) -> CString {
    let s = format_message(record) + "\n";
    let escaped = s.replace("%", "%%").replace("\\", "\\\\");
    let cs = CString::new(escaped).expect("no null in message");
    cs
}

impl log::Log for LottoLogger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        metadata.level() == LevelFilter::Trace
            || metadata.level() as usize >= LEVEL.load(Ordering::Relaxed)
    }

    fn log(&self, record: &log::Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        // Safety: the string is escaped, and TRACE_REDIRECT is
        // thread-safe.
        match record.level() {
            log::Level::Error => unsafe {
                raw::logger_errorf(format_message_escape_cstr(record).as_ptr());
            },
            log::Level::Warn => unsafe {
                raw::logger_warnf(format_message_escape_cstr(record).as_ptr());
            },
            log::Level::Info => unsafe {
                raw::logger_infof(format_message_escape_cstr(record).as_ptr());
            },
            log::Level::Debug => unsafe {
                raw::logger_debugf(format_message_escape_cstr(record).as_ptr());
            },
            log::Level::Trace => {
                TRACE_REDIRECT
                    .try_lock()
                    .expect("single thread")
                    .output(record);
            }
        }
    }

    fn flush(&self) {
        TRACE_REDIRECT.try_lock().expect("single thread").flush();
    }
}
