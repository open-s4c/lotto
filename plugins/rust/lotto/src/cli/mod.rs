pub mod args;
pub mod flags;

pub use args::Args;
pub use flags::FlagKey;

use crate::base::Flags;
use lotto_sys as raw;
use std::ffi::CString;
use std::num::NonZeroI32;
use std::path::Path;

pub fn preload<P: AsRef<Path>>(
    tempdir: P,
    verbose: bool,
    plotto: bool,
    flotto: bool,
    memmgr_chain_runtime: &str,
    memmgr_chain_user: &str,
) {
    let osstr = tempdir.as_ref().as_os_str();
    let tempdir_cstr = CString::new(osstr.as_encoded_bytes()).unwrap();
    let memmgr_chain_runtime_cstr = CString::new(memmgr_chain_runtime).unwrap();
    let memmgr_chain_user_cstr = CString::new(memmgr_chain_user).unwrap();
    unsafe {
        raw::preload(
            tempdir_cstr.as_ptr(),
            verbose,
            plotto,
            flotto,
            memmgr_chain_runtime_cstr.as_ptr(),
            memmgr_chain_user_cstr.as_ptr(),
        )
    }
}

pub fn cli_trace_last_clk<P: AsRef<Path>>(path: P) -> u64 {
    let osstr = path.as_ref().as_os_str();
    let cstr = CString::new(osstr.as_encoded_bytes()).unwrap();
    unsafe { raw::cli_trace_last_clk(cstr.as_ptr()) }
}

pub fn adjust<P: AsRef<Path>>(path: P) -> bool {
    let osstr = path.as_ref().as_os_str();
    let cstr = CString::new(osstr.as_encoded_bytes()).unwrap();
    unsafe { raw::adjust(cstr.as_ptr()) }
}

pub fn execute(args: &Args, flags: &Flags, config: bool) -> i32 {
    let args = args.as_ptr();
    let flags = flags.as_ptr();
    unsafe { raw::execute(args, flags, config) }
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct ErrorCode(pub NonZeroI32);

pub type SubCmdResult = Result<(), ErrorCode>;

impl<T: HasErrorCode> From<T> for ErrorCode {
    fn from(value: T) -> Self {
        ErrorCode(value.error_code())
    }
}

pub trait HasErrorCode {
    fn error_code(&self) -> NonZeroI32;
}

impl HasErrorCode for i32 {
    fn error_code(&self) -> NonZeroI32 {
        if *self == 0 {
            panic!("error code cannot be zero");
        }
        unsafe { NonZeroI32::new_unchecked(*self) }
    }
}

impl From<NonZeroI32> for ErrorCode {
    fn from(value: NonZeroI32) -> Self {
        ErrorCode(value)
    }
}

pub trait FromErrorCode {
    fn from_error_code(code: i32) -> Self;
}

impl FromErrorCode for SubCmdResult {
    fn from_error_code(code: i32) -> SubCmdResult {
        if code == 0 {
            Ok(())
        } else {
            Err(ErrorCode(NonZeroI32::new(code).unwrap()))
        }
    }
}
