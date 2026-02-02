use crate::base::{Clock, Record};
use crate::owned::Owned;
use crate::sys::stream::Stream;
use lotto_sys as raw;
use std::ffi::CString;
use std::{os::unix::ffi::OsStrExt, path::Path};

crate::wrap_with_dispose!(Trace, raw::trace_t, raw::trace_destroy);

impl Trace {
    pub fn new_file(st: &mut Stream) -> Owned<Trace> {
        unsafe {
            let t = raw::trace_file_create(st.as_mut_ptr());
            Owned::from_raw(t)
        }
    }

    pub fn load_file<P: AsRef<Path>>(path: P) -> Owned<Trace> {
        let cstr = CString::new(path.as_ref().as_os_str().as_bytes()).unwrap();
        let ptr = unsafe { raw::cli_trace_load(cstr.as_ptr()) };
        unsafe { Owned::from_raw(ptr) }
    }

    pub fn load(&mut self) {
        unsafe {
            raw::trace_load(&mut self.inner as *mut _);
        }
    }

    pub fn save<P: AsRef<Path>>(&self, path: P) {
        let cstr = CString::new(path.as_ref().as_os_str().as_bytes()).unwrap();
        unsafe {
            raw::cli_trace_save(&self.inner as *const _, cstr.as_ptr());
        }
    }

    pub fn advance(&mut self) {
        unsafe {
            raw::trace_advance(&mut self.inner as *mut _);
        }
    }

    pub fn append(&mut self, r: Owned<Record>) -> Result<(), ()> {
        unsafe {
            let ret = raw::trace_append(&mut self.inner as *mut _, r.into_raw());
            if ret == raw::TRACE_OK as i32 {
                Ok(())
            } else {
                Err(())
            }
        }
    }

    pub fn append_safe(&mut self, r: Owned<Record>) -> Result<(), ()> {
        unsafe {
            let ret = raw::trace_append_safe(&mut self.inner as *mut _, r.into_raw());
            if ret == raw::TRACE_OK as i32 {
                Ok(())
            } else {
                Err(())
            }
        }
    }

    pub fn next_any(&mut self) -> Option<&Record> {
        self.next(raw::record(!0))
    }

    pub fn next_any_mut(&mut self) -> Option<&mut Record> {
        self.next_mut(raw::record(!0))
    }

    pub fn next(&mut self, kind: raw::record) -> Option<&Record> {
        unsafe {
            let ptr = raw::trace_next(&mut self.inner as *mut _, kind);
            if ptr.is_null() {
                return None;
            }
            Some(&*(ptr as *const Record))
        }
    }

    pub fn next_mut(&mut self, kind: raw::record) -> Option<&mut Record> {
        unsafe {
            let ptr = raw::trace_next(&mut self.inner as *mut _, kind);
            if ptr.is_null() {
                return None;
            }
            Some(&mut *(ptr as *mut Record))
        }
    }

    pub fn last(&mut self) -> Option<&Record> {
        unsafe {
            let ptr = raw::trace_last(&mut self.inner as *mut _);
            if ptr.is_null() {
                return None;
            }
            Some(&*(ptr as *const Record))
        }
    }

    pub fn last_mut(&mut self) -> Option<&mut Record> {
        unsafe {
            let ptr = raw::trace_last(&mut self.inner as *mut _);
            if ptr.is_null() {
                return None;
            }
            Some(&mut *(ptr as *mut Record))
        }
    }

    pub fn forget(&mut self) {
        unsafe {
            raw::trace_forget(&mut self.inner as *mut _);
        }
    }

    pub fn trim_to_goal(&mut self, goal: Clock, drop_old_config: bool) {
        unsafe {
            raw::cli_trace_trim_to_goal(self.as_mut_ptr(), goal, drop_old_config);
        }
    }
}
