use crate::base::{AsFlag, Flag, Value};
use lotto_sys as raw;
use std::ffi::{c_char, CStr};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Once;

type FlagCallbackFn = raw::flag_callback_f;

/// The key used for accessing a CLI flag.
///
/// This is the low-level mechanism for declaring a new flag.
///
/// This key is initialized once when it's first accessed. Therefore,
/// if it's not included in the registration of a subcmd, any access
/// to it is invalid.
pub struct FlagKey {
    // Descriptions
    name: *const c_char,
    opt: *const c_char,
    long_opt: *const c_char,
    help: *const c_char,
    desc: *const c_char,
    default: DefaultValue,
    str_converter: &'static StrConverter,
    callback: FlagCallbackFn,

    // Singleton
    key: AtomicU64,
    once: Once,
}

// Safety: FlagKey is read-only after initialization.
unsafe impl Sync for FlagKey {}

impl FlagKey {
    /// Declare a new flag.
    pub const fn new(
        name: &'static CStr,
        opt: &'static CStr,
        long_opt: &'static CStr,
        help: &'static CStr,
        desc: &'static CStr,
        default: Value<'static>,
        str_converter: &'static StrConverter,
        callback: FlagCallbackFn,
    ) -> FlagKey {
        FlagKey {
            name: name.as_ptr(),
            opt: opt.as_ptr(),
            long_opt: long_opt.as_ptr(),
            help: help.as_ptr(),
            desc: desc.as_ptr(),
            default: DefaultValue::Imm(default),
            str_converter,
            callback,
            key: AtomicU64::new(0),
            once: Once::new(),
        }
    }

    /// Declare a new flag with deferred default value computation.
    pub const fn new_defer_default(
        name: &'static CStr,
        opt: &'static CStr,
        long_opt: &'static CStr,
        help: &'static CStr,
        desc: &'static CStr,
        default: fn() -> Value<'static>,
        str_converter: &'static StrConverter,
        callback: FlagCallbackFn,
    ) -> FlagKey {
        FlagKey {
            name: name.as_ptr(),
            opt: opt.as_ptr(),
            long_opt: long_opt.as_ptr(),
            help: help.as_ptr(),
            desc: desc.as_ptr(),
            str_converter,
            default: DefaultValue::Defer(default),
            callback,
            key: AtomicU64::new(0),
            once: Once::new(),
        }
    }

    /// Declare a new flag with deferred default value computation (raw value).
    pub const fn new_defer_default_raw(
        name: &'static CStr,
        opt: &'static CStr,
        long_opt: &'static CStr,
        help: &'static CStr,
        desc: &'static CStr,
        default: fn() -> raw::value,
        str_converter: &'static StrConverter,
        callback: FlagCallbackFn,
    ) -> FlagKey {
        FlagKey {
            name: name.as_ptr(),
            opt: opt.as_ptr(),
            long_opt: long_opt.as_ptr(),
            help: help.as_ptr(),
            desc: desc.as_ptr(),
            str_converter,
            default: DefaultValue::DeferRaw(default),
            callback,
            key: AtomicU64::new(0),
            once: Once::new(),
        }
    }

    /// Get or initialize the [FlagKey].
    pub fn get(&self) -> Flag {
        let val = match self.default {
            DefaultValue::Imm(v) => v.to_raw(),
            DefaultValue::Defer(f) => f().to_raw(),
            DefaultValue::DeferRaw(f) => f(),
        };
        let flag_val = raw::flag_val {
            _val: val,
            is_default: true,
            force_no_default: false,
        };

        self.once.call_once(|| {
            let key = unsafe {
                raw::new_flag(
                    self.name,
                    self.opt,
                    self.long_opt,
                    self.help,
                    self.desc,
                    flag_val,
                    self.str_converter.to_raw(),
                    self.callback,
                )
            };
            self.key.store(key as u64, Ordering::Relaxed);
        });
        self.key.load(Ordering::Relaxed) as Flag
    }
}

enum DefaultValue {
    Imm(Value<'static>),
    Defer(fn() -> Value<'static>),
    DeferRaw(fn() -> raw::value),
}

#[cfg(feature = "qlotto")]
const CREP_DEFAULT: bool = true;

#[cfg(not(feature = "qlotto"))]
const CREP_DEFAULT: bool = false;

pub static FLAG_CREP: FlagKey = FlagKey::new(
    c"FLAG_CREP",
    c"",
    c"crep",
    c"",
    c"enable crep",
    Value::Bool(CREP_DEFAULT),
    &StrConverter::None,
    None,
);

pub static FLAG_VERBOSE: FlagKey = FlagKey::new(
    c"FLAG_VERBOSE",
    c"v",
    c"verbose",
    c"",
    c"verbose",
    Value::Bool(false),
    &StrConverter::None,
    None,
);

pub static FLAG_INPUT: FlagKey = FlagKey::new(
    c"FLAG_INPUT",
    c"i",
    c"input-trace",
    c"FILE",
    c"input trace FILE",
    Value::Sval(c"replay.trace"),
    &StrConverter::None,
    None,
);

pub static FLAG_OUTPUT: FlagKey = FlagKey::new(
    c"FLAG_OUTPUT",
    c"o",
    c"output-trace",
    c"FILE",
    c"output trace FILE",
    Value::Sval(c"replay.trace"),
    &StrConverter::None,
    None,
);

pub static FLAG_REPLAY_GOAL: FlagKey = FlagKey::new(
    c"FLAG_REPLAY_GOAL",
    c"g",
    c"goal",
    c"INT",
    c"replay goal",
    Value::U64(u64::MAX),
    &StrConverter::None,
    None,
);

pub static FLAG_TEMPORARY_DIRECTORY: FlagKey = FlagKey::new_defer_default_raw(
    c"FLAG_TEMPORARY_DIRECTORY",
    c"",
    c"temporary-directory",
    c"DIR",
    c"temporary directory to write Lotto files",
    || Value::Sval(unsafe { CStr::from_ptr(raw::get_default_temporary_directory()) }).to_raw(),
    &StrConverter::None,
    None,
);

pub static FLAG_NO_PRELOAD: FlagKey = FlagKey::new(
    c"FLAG_NO_PRELOAD",
    c"",
    c"no-preload",
    c"",
    c"does not preload lotto shared library, used for qlotto",
    Value::Bool(false),
    &StrConverter::None,
    None,
);

pub static FLAG_LOGGER_BLOCK: FlagKey = FlagKey::new(
    c"FLAG_LOGGER_BLOCK",
    c"",
    c"logger-block",
    c"NAME1|NAME2|...",
    c"set file name without suffix to disable logger message",
    Value::Sval(c""),
    &StrConverter::None,
    None,
);

pub static FLAG_BEFORE_RUN: FlagKey = FlagKey::new(
    c"FLAG_BEFORE_RUN",
    c"",
    c"before_run",
    c"CMD",
    c"action to be executed before each run",
    Value::Sval(c""),
    &StrConverter::None,
    None,
);

pub static FLAG_AFTER_RUN: FlagKey = FlagKey::new(
    c"FLAG_AFTER_RUN",
    c"",
    c"after-run",
    c"CMD",
    c"action to be executed after each run",
    Value::Sval(c""),
    &StrConverter::None,
    None,
);

pub static FLAG_LOGGER_FILE: FlagKey = FlagKey::new(
    c"FLAG_LOGGER_FILE",
    c"",
    c"log",
    c"FILE",
    c"output log FILE",
    Value::Sval(c"stderr"),
    &StrConverter::None,
    None,
);

pub static FLAG_ROUNDS: FlagKey = FlagKey::new(
    c"FLAG_ROUNDS",
    c"r",
    c"rounds",
    c"INT",
    c"maximum number of rounds",
    Value::U64(u64::MAX),
    &StrConverter::None,
    None,
);

#[derive(Clone)]
pub enum StrConverter {
    None,
    BitsGet {
        str_: raw::bits_str_get,
        from_: raw::bits_from,
        size_: usize,
        help_: raw::bits_str_help,
    },
    BitsPrint {
        str_: raw::bits_str_print,
        from_: raw::bits_from,
        size_: usize,
        help_: raw::bits_str_help,
    },
    Bool {
        str_: raw::bool_str,
        from_: raw::bool_from,
        help_: &'static CStr,
    },
}

impl StrConverter {
    pub fn to_raw(&self) -> raw::str_converter_t {
        match self {
            StrConverter::None => raw::str_converter_t {
                _type: raw::str_converter_type::STR_CONVERTER_TYPE_NONE,
                __bindgen_anon_1: raw::str_converter_s__bindgen_ty_1 {
                    _bool: raw::str_converter_s__bindgen_ty_1__bindgen_ty_3 {
                        str_: None,
                        from: None,
                        help: std::ptr::null(),
                    },
                },
            },
            StrConverter::BitsGet {
                str_,
                from_,
                size_,
                help_,
            } => raw::str_converter_t {
                _type: raw::str_converter_type::STR_CONVERTER_TYPE_BITS_GET,
                __bindgen_anon_1: raw::str_converter_s__bindgen_ty_1 {
                    _bits_get: raw::str_converter_s__bindgen_ty_1__bindgen_ty_1 {
                        str_: str_.clone(),
                        from: from_.clone(),
                        size: size_.clone(),
                        help: help_.clone(),
                    },
                },
            },
            StrConverter::BitsPrint {
                str_,
                from_,
                size_,
                help_,
            } => raw::str_converter_t {
                _type: raw::str_converter_type::STR_CONVERTER_TYPE_BITS_PRINT,
                __bindgen_anon_1: raw::str_converter_s__bindgen_ty_1 {
                    _bits_print: raw::str_converter_s__bindgen_ty_1__bindgen_ty_2 {
                        str_: str_.clone(),
                        from: from_.clone(),
                        size: size_.clone(),
                        help: help_.clone(),
                    },
                },
            },
            StrConverter::Bool { str_, from_, help_ } => raw::str_converter_t {
                _type: raw::str_converter_type::STR_CONVERTER_TYPE_BOOL,
                __bindgen_anon_1: raw::str_converter_s__bindgen_ty_1 {
                    _bool: raw::str_converter_s__bindgen_ty_1__bindgen_ty_3 {
                        str_: str_.clone(),
                        from: from_.clone(),
                        help: help_.as_ptr(),
                    },
                },
            },
        }
    }

    pub fn from_raw(raw: raw::str_converter_t) -> StrConverter {
        unsafe {
            match raw._type {
                raw::str_converter_type::STR_CONVERTER_TYPE_NONE => StrConverter::None,
                raw::str_converter_type::STR_CONVERTER_TYPE_BITS_GET => StrConverter::BitsGet {
                    str_: raw.__bindgen_anon_1._bits_get.str_,
                    from_: raw.__bindgen_anon_1._bits_get.from,
                    size_: raw.__bindgen_anon_1._bits_get.size,
                    help_: raw.__bindgen_anon_1._bits_get.help,
                },
                raw::str_converter_type::STR_CONVERTER_TYPE_BITS_PRINT => StrConverter::BitsPrint {
                    str_: raw.__bindgen_anon_1._bits_print.str_,
                    from_: raw.__bindgen_anon_1._bits_print.from,
                    size_: raw.__bindgen_anon_1._bits_print.size,
                    help_: raw.__bindgen_anon_1._bits_print.help,
                },
                raw::str_converter_type::STR_CONVERTER_TYPE_BOOL => StrConverter::Bool {
                    str_: raw.__bindgen_anon_1._bool.str_,
                    from_: raw.__bindgen_anon_1._bool.from,
                    help_: CStr::from_ptr(raw.__bindgen_anon_1._bool.help),
                },
            }
        }
    }
}

pub static STR_CONVERTER_NONE: StrConverter = StrConverter::None;

pub static STR_CONVERTER_BOOL: StrConverter = StrConverter::Bool {
    str_: Some(raw::enabled_str),
    from_: Some(raw::enabled_from),
    help_: c"enable|disable",
};

impl AsFlag for FlagKey {
    fn as_flag(&self) -> Flag {
        self.get()
    }
}

pub fn init() {
    let _ = FLAG_CREP.get();
    let _ = FLAG_VERBOSE.get();
    let _ = FLAG_INPUT.get();
    let _ = FLAG_OUTPUT.get();
    let _ = FLAG_REPLAY_GOAL.get();
    let _ = FLAG_TEMPORARY_DIRECTORY.get();
    let _ = FLAG_NO_PRELOAD.get();
    let _ = FLAG_LOGGER_BLOCK.get();
    let _ = FLAG_BEFORE_RUN.get();
    let _ = FLAG_AFTER_RUN.get();
    let _ = FLAG_LOGGER_FILE.get();
    let _ = FLAG_ROUNDS.get();
}
