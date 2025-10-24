use crate::owned::{Dispose, Owned};
use lotto_sys as raw;
use std::{ffi::CString, os::unix::ffi::OsStrExt, path::Path};

crate::wrap!(Stream, raw::stream_t);

impl Dispose for Stream {
    type Source = raw::stream_t;

    unsafe fn dispose(ptr: *mut raw::stream_t) {
        raw::stream_close(ptr);
        raw::sys_free(ptr as *mut _);
    }
}

impl Stream {
    pub fn new_file_out<P: AsRef<Path>>(path: P) -> Owned<Stream> {
        let path = CString::new(path.as_ref().as_os_str().as_bytes()).unwrap();
        unsafe {
            let ptr = raw::stream_file_alloc();
            raw::stream_file_out(ptr, path.as_ptr());
            Owned::from_raw(ptr)
        }
    }

    pub fn new_file_in<P: AsRef<Path>>(path: P) -> Owned<Stream> {
        let path = CString::new(path.as_ref().as_os_str().as_bytes()).unwrap();
        unsafe {
            let ptr = raw::stream_file_alloc();
            raw::stream_file_in(ptr, path.as_ptr());
            Owned::from_raw(ptr)
        }
    }
}
