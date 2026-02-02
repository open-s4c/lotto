use std::{
    backtrace::{Backtrace, BacktraceStatus},
    panic::{self, PanicHookInfo},
};

fn lotto_rust_panic_hook(info: &PanicHookInfo) {
    match info.location() {
        Some(loc) => {
            eprintln!(
                "Lotto-Rust panicked in {} at line {}:",
                loc.file(),
                loc.line()
            );
        }
        None => {
            eprintln!("Lotto-Rust panicked (unknown location):");
        }
    }
    if let Some(s) = info.payload().downcast_ref::<&str>() {
        eprintln!("{}", s);
    } else if let Some(s) = info.payload().downcast_ref::<String>() {
        eprintln!("{}", s);
    } else {
        eprintln!("<invalid panic payload>");
    }

    let backtrace = Backtrace::capture();
    if let BacktraceStatus::Captured = backtrace.status() {
        eprintln!("{}", backtrace);
    }
}

/// Initialize Lotto's panic handler.
///
/// The handler will redirect the message to Lotto's logger.
pub fn init() {
    panic::set_hook(Box::new(lotto_rust_panic_hook));
}
