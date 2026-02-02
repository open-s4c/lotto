use crate::owned::Owned;
use std::{ffi::CStr, path::PathBuf};

use crate::base::value::Value;
use lotto_sys as raw;

crate::wrap_with_dispose!(Flags, raw::flags_t, raw::flags_free);

impl ToOwned for Flags {
    type Owned = Owned<Flags>;

    fn to_owned(&self) -> Owned<Flags> {
        let mut result = Flags::new();
        unsafe {
            raw::flags_cpy(result.as_mut_ptr(), self.as_ptr());
        }
        result
    }
}

impl Flags {
    pub fn new() -> Owned<Flags> {
        unsafe {
            let ptr = raw::flagmgr_flags_alloc();
            Owned::from_raw(ptr)
        }
    }

    #[allow(clippy::should_implement_trait)]
    pub fn default() -> &'static Flags {
        unsafe {
            let ptr = raw::flags_default();
            &*(ptr as *const Flags)
        }
    }

    pub fn get_value<'a, F: AsFlag>(&'a self, f: &F) -> Value<'a> {
        unsafe { raw::flags_get_value(self.as_ptr(), f.as_flag()).into() }
    }

    /// # Panics
    ///
    /// Panic when the flag is not a sval.
    pub fn get_csval<F: AsFlag>(&self, f: &F) -> &CStr {
        match self.get_value(f) {
            Value::Sval(cs) => cs,
            _ => panic!("flag is not sval"),
        }
    }

    /// # Panics
    ///
    /// Panic when the flag is not a sval, or does not contain valid utf-8.
    pub fn get_sval<F: AsFlag>(&self, f: &F) -> &str {
        self.get_csval(f).to_str().ok().unwrap()
    }

    /// # Panics
    ///
    /// Panic when the flag is not a sval, or is NULL, or does not
    /// contain valid utf-8.
    pub fn get_path_buf<F: AsFlag>(&self, f: &F) -> PathBuf {
        self.get_value(f).to_path_buf()
    }

    /// # Panics
    ///
    /// Panic when the flag is not a uval.
    pub fn get_uval<F: AsFlag>(&self, f: &F) -> u64 {
        self.get_value(f).as_u64()
    }

    /// # Panics
    ///
    /// Panic when the flag is not a bval.
    pub fn is_on<F: AsFlag>(&self, f: &F) -> bool {
        self.get_value(f).is_on()
    }

    /// # Panics
    ///
    /// Panic when flag is not any.
    ///
    /// # Safety
    ///
    /// Only valid if it is really a `T`.
    ///
    /// You should also check if you need to drop it.
    pub unsafe fn get_any<'a, T, F: AsFlag>(&self, f: &F) -> &'a T {
        let p = self.get_value(f).as_any();
        unsafe { &*(p as *const T) }
    }

    /// # Panics
    ///
    /// Panic when flag is not any.
    ///
    /// # Safety
    ///
    /// Only valid if it is really a `T`.
    ///
    /// You should also check if you need to drop it.
    pub unsafe fn get_any_mut<'a, T, F: AsFlag>(&self, f: &F) -> &'a mut T {
        let p = self.get_value(f).as_any();
        unsafe { &mut *(p as *mut T) }
    }

    pub fn set_default<F: AsFlag>(&mut self, f: &F, val: Value) {
        unsafe { raw::flags_set_default(self.as_mut_ptr(), f.as_flag(), val.to_raw()) }
    }

    pub fn set_by_opt<F: AsFlag>(&mut self, f: &F, val: Value) {
        unsafe { raw::flags_set_by_opt(self.as_mut_ptr(), f.as_flag(), val.to_raw()) }
    }
}

/// Flag number.
pub type Flag = raw::flag_t;

/// Flag selector.
pub type FlagSel = Vec<Flag>;

pub trait AsFlag {
    fn as_flag(&self) -> Flag;
}

impl AsFlag for raw::flag_t {
    fn as_flag(&self) -> Flag {
        *self
    }
}
