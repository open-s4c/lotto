use lotto_sys as raw;
use std::ffi;
use std::ffi::CStr;
use std::path::PathBuf;

/// Wrapped [`raw::value`].
#[derive(Debug, Clone, Copy)]
pub enum Value<'a> {
    None,
    U64(u64),
    Bool(bool),
    Sval(&'a CStr),
    ANY(*const ffi::c_void),
}

impl<'a> Value<'a> {
    pub fn to_raw(&self) -> raw::value {
        let (ty, union) = match *self {
            Value::None => (
                raw::value_type_VALUE_TYPE_NONE,
                raw::value__bindgen_ty_1 { _uval: 0 },
            ),
            Value::U64(x) => (
                raw::value_type_VALUE_TYPE_UINT64,
                raw::value__bindgen_ty_1 { _uval: x },
            ),
            Value::Bool(x) => (
                raw::value_type_VALUE_TYPE_BOOL,
                raw::value__bindgen_ty_1 { _bval: x },
            ),
            Value::Sval(x) => (
                raw::value_type_VALUE_TYPE_STRING,
                raw::value__bindgen_ty_1 { _sval: x.as_ptr() },
            ),
            Value::ANY(x) => (
                raw::value_type_VALUE_TYPE_ANY,
                raw::value__bindgen_ty_1 { _any: x },
            ),
        };
        raw::value {
            _type: ty,
            __bindgen_anon_1: union,
        }
    }
}

impl<'a> From<raw::value> for Value<'a> {
    fn from(val: raw::value) -> Self {
        match val._type {
            raw::value_type_VALUE_TYPE_NONE => Value::None,
            raw::value_type_VALUE_TYPE_BOOL => {
                Value::Bool(unsafe {
                    // `C` doesn't guarantee that a `bool` is either `0` or `1`, but
                    // rust requires exactly this. So to prevent UB we read the raw `u8`
                    // representation and convert that to a rust bool.
                    let b = val.__bindgen_anon_1._u8val;
                    b != 0
                })
            }
            raw::value_type_VALUE_TYPE_UINT64 => Value::U64(unsafe { val.__bindgen_anon_1._uval }),
            raw::value_type_VALUE_TYPE_STRING => Value::Sval(unsafe {
                let ptr = val.__bindgen_anon_1._sval;
                if ptr.is_null() {
                    panic!("null string pointer");
                }
                CStr::from_ptr(ptr)
            }),
            raw::value_type_VALUE_TYPE_ANY => Value::ANY(unsafe { val.__bindgen_anon_1._any }),
            _ => {
                panic!("Unknown `_type` variant {:?} for `raw::value`", val._type);
            }
        }
    }
}

impl<'a> Value<'a> {
    /// # Panics
    ///
    /// Panics if the value is not a bool.
    pub fn is_on(&self) -> bool {
        match self {
            Value::Bool(b) => *b,
            _ => panic!("value is not bool"),
        }
    }

    /// # Panics
    ///
    /// Panic if the value is not ANY.
    pub fn as_any(&self) -> *const ffi::c_void {
        match self {
            Value::ANY(x) => *x,
            _ => panic!("value is not any"),
        }
    }

    /// # Panics
    ///
    /// Panic if the value is not u64.
    pub fn as_u64(&self) -> u64 {
        match self {
            Value::U64(x) => *x,
            _ => panic!("value is not u64"),
        }
    }

    /// # Panics
    ///
    /// Panic if the value is not u64.
    pub fn as_uval(&self) -> u64 {
        self.as_u64()
    }

    /// # Panics
    ///
    /// Panic if (1) the value is not Sval, or (2) the pointer is null.
    pub fn as_csval(&self) -> &CStr {
        match self {
            Value::Sval(x) => *x,
            _ => panic!("value is not sval"),
        }
    }

    /// # Panics
    ///
    /// Panic if the value is not Sval, or it's NULL, or does not
    /// contain valid utf-8.
    pub fn as_sval(&self) -> &str {
        self.as_csval().to_str().ok().unwrap()
    }

    /// # Panics
    ///
    /// Panic if the value is not Sval, or it's NULL, or does not
    /// contain valid utf-8.
    pub fn to_path_buf(&self) -> PathBuf {
        PathBuf::from(self.as_sval())
    }
}
