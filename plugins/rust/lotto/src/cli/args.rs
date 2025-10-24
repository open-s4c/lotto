use lotto_sys as raw;
use std::borrow::Cow;
use std::ffi::{c_char, CStr};
use std::fmt;
use std::marker::PhantomData;

use crate::owned::Owned;

crate::wrap_with_dispose!(Args, raw::args_t, _args_destroy);

impl ToOwned for Args {
    type Owned = Owned<Args>;
    fn to_owned(&self) -> Self::Owned {
        let copy = _args_clone(self.as_ptr());
        // Safety: It is ok to call `_args_destroy` on copy.
        unsafe { Owned::from_raw(copy) }
    }
}

impl std::fmt::Debug for Args {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let cstrs: Vec<_> = self.cstrs().map(CStr::to_string_lossy).collect();
        write!(f, "{:?}", cstrs)
    }
}

impl Args {
    pub fn print(&self) {
        unsafe {
            raw::args_print(&self.inner);
        }
    }

    pub fn shift(&mut self, n: i32) {
        unsafe {
            raw::args_shift(&mut self.inner, n);
        }
    }

    pub fn cstrs(&self) -> impl Iterator<Item = &CStr> {
        CStrs {
            argc: self.inner.argc as usize,
            argv: self.inner.argv,
            i: 0,
            phantom: PhantomData,
        }
    }

    pub fn strs_strict(&self) -> impl Iterator<Item = Option<&str>> {
        self.cstrs().map(|cstr| cstr.to_str().ok())
    }

    pub fn strs(&self) -> impl Iterator<Item = Cow<str>> {
        self.cstrs().map(|cstr| cstr.to_string_lossy())
    }
}

pub struct CStrs<'a> {
    argc: usize,
    argv: *mut *mut c_char,
    i: usize,
    phantom: PhantomData<&'a Args>,
}

impl<'a> Iterator for CStrs<'a> {
    type Item = &'a CStr;
    fn next(&mut self) -> Option<&'a CStr> {
        if self.i >= self.argc {
            return None;
        }
        let cur = unsafe { CStr::from_ptr(*self.argv.add(self.i)) };
        self.i += 1;
        Some(cur)
    }
}

impl fmt::Display for Args {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let cstrs: Vec<_> = self.strs().collect();
        write!(f, "{}", cstrs.join(" "))
    }
}

/// Deep copy [`raw::args_t`].
fn _args_clone(src: *const raw::args_t) -> *mut raw::args_t {
    unsafe {
        let dst = raw::sys_calloc(1, std::mem::size_of::<raw::args_t>()) as *mut raw::args_t;
        (*dst).argc = (*src).argc;
        (*dst).arg0 = raw::sys_strdup((*src).arg0);
        (*dst).argv = raw::sys_calloc((*src).argc as usize + 1, std::mem::size_of::<*mut c_char>())
            as *mut *mut c_char;
        for i in 0..(*src).argc {
            *(*dst).argv.add(i as usize) = raw::sys_strdup(*(*src).argv.add(i as usize));
        }
        *(*dst).argv.add((*src).argc as usize) = std::ptr::null_mut();
        dst
    }
}

/// NOTE: Only use on those created from [`_args_clone`].
unsafe fn _args_destroy(a: *mut raw::args_t) {
    use std::ffi::c_void;
    unsafe {
        raw::sys_free((*a).arg0 as *mut c_void);
        let argv = std::slice::from_raw_parts_mut((*a).argv, (*a).argc as usize + 1);
        argv.iter().for_each(|s| raw::sys_free(*s as *mut c_void));
        raw::sys_free((*a).argv as *mut c_void);
        raw::sys_free(a as *mut c_void);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    unsafe fn create(argc: usize) -> Owned<Args> {
        let argv = raw::sys_calloc(argc + 1, std::mem::size_of::<*mut i8>()) as *mut *mut i8;
        for i in 0..argc {
            *argv.add(i) = raw::sys_strdup(c"ARGUMENT".as_ptr());
        }
        let ret = raw::args_t {
            argc: argc as i32,
            argv,
            arg0: raw::sys_strdup(c"ARG0".as_ptr()),
        };
        let ptr = Box::into_raw(Box::new(ret));
        unsafe { Owned::from_raw(ptr) }
    }

    #[test]
    fn copy_0() {
        let src = unsafe { create(0) };
        let dst = src.clone();
        drop(dst);
    }

    #[test]
    fn copy_1000() {
        let src = unsafe { create(1000) };
        let dst = src.clone();
        drop(dst);
    }
}
