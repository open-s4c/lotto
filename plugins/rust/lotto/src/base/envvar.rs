use std::{
    env,
    ffi::{CStr, CString, OsStr, OsString},
    os::unix::ffi::OsStrExt,
};

/// Temporarily set an environment variable and restore the old value
/// when it goes out of scope.
pub struct EnvScope {
    key: OsString,
    old: Option<OsString>,
}

impl EnvScope {
    pub fn new<K: AsRef<OsStr>, V: AsRef<OsStr>>(key: K, new: V) -> EnvScope {
        let old = env::var_os(key.as_ref());
        env::set_var(key.as_ref(), new.as_ref());
        EnvScope {
            key: key.as_ref().to_os_string(),
            old,
        }
    }

    pub fn unset<K: AsRef<OsStr>>(key: K) -> EnvScope {
        let key = key.as_ref();
        let old = env::var_os(key);
        env::remove_var(key);
        EnvScope {
            key: key.to_os_string(),
            old,
        }
    }
}

impl Drop for EnvScope {
    fn drop(&mut self) {
        if let Some(old) = &self.old {
            env::set_var(&self.key, old);
        } else {
            env::remove_var(&self.key);
        }
    }
}

/// Retrieve an environmental variable.
///
/// # Panics
///
/// This function panics should there are any errors, *including* non-existence.
pub fn must_get(key: &str) -> String {
    match get(key) {
        Some(val) => val,
        None => panic!("Envvar {} does not exist", key),
    }
}

/// Retrieve an environmental variable.
///
/// # Panics
///
/// This function panics should there are any errors except non-existence.
pub fn get(key: &str) -> Option<String> {
    match env::var(key) {
        Ok(val) => Some(val),
        Err(env::VarError::NotPresent) => None,
        Err(e) => panic!("Error when getting envvar '{}': {}", key, e),
    }
}

/// # Safety
///
/// Only use this in the CLI commands.
#[macro_export]
macro_rules! envvar_set {
    ($($key:expr => $value:expr),* $(,)?) => {
        $(::std::env::set_var($key, $crate::base::envvar::EnvVal::into_env_val($value)));*
    };
}

pub trait EnvVal {
    fn into_env_val(self) -> OsString;
}

impl EnvVal for crate::base::Value<'_> {
    fn into_env_val(self) -> OsString {
        use crate::base::Value::*;
        match self {
            U64(val) => format!("{}", val).into(),
            Sval(cs) => cs.into_env_val(),
            _ => {
                panic!("EnvVal only accepts U64 and Sval")
            }
        }
    }
}

impl EnvVal for u64 {
    fn into_env_val(self) -> OsString {
        format!("{}", self).into()
    }
}

impl EnvVal for &str {
    fn into_env_val(self) -> OsString {
        self.to_string().into()
    }
}

impl EnvVal for String {
    fn into_env_val(self) -> OsString {
        self.into()
    }
}

impl EnvVal for &'_ CStr {
    fn into_env_val(self) -> OsString {
        OsStr::from_bytes(self.to_bytes()).to_os_string()
    }
}

impl EnvVal for CString {
    fn into_env_val(self) -> OsString {
        OsStr::from_bytes(self.to_bytes()).to_os_string()
    }
}
