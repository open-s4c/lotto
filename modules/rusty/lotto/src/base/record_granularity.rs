use lotto_sys::{self as raw, RECORD_GRANULARITIES_MAX_LEN};
use std::convert::Infallible;
use std::ffi::{CStr, CString};
use std::fmt;
use std::str::FromStr;

pub use raw::record_granularity::*;

/// A bitset of [`lotto_sys::record_granularity`].
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(transparent)]
pub struct RecordGranularities(u64);

impl fmt::Display for RecordGranularities {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        let mut buf = [0; RECORD_GRANULARITIES_MAX_LEN as usize];
        unsafe {
            raw::record_granularities_str(self.0, buf.as_mut_ptr());
        }
        let cstr = unsafe { CStr::from_ptr(&buf as *const i8) };
        write!(formatter, "{}", cstr.to_str().unwrap())
    }
}

impl FromStr for RecordGranularities {
    type Err = Infallible;
    fn from_str(s: &str) -> Result<RecordGranularities, Self::Err> {
        let cstr = CString::new(s).unwrap();
        let res = unsafe { raw::record_granularities_from(cstr.as_c_str().as_ptr()) };
        Ok(Self(res))
    }
}

impl From<u64> for RecordGranularities {
    fn from(value: u64) -> RecordGranularities {
        RecordGranularities(value)
    }
}

impl From<raw::record_granularity> for RecordGranularities {
    fn from(value: raw::record_granularity) -> RecordGranularities {
        RecordGranularities(value as u64)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn conversion() {
        let g = RECORD_GRANULARITY_CAPTURE;
        let gs = RecordGranularities(g as u64);
        let s = gs.to_string();
        assert_eq!(s, "CAPTURE");
        let gs_from_s = RecordGranularities::from_str(&s).unwrap();
        assert_eq!(gs, gs_from_s);
    }

    #[test]
    fn bits() {
        let gs_from_s = RecordGranularities::from_str("CAPTURE|CHPT").unwrap();
        assert_eq!(gs_from_s.0, 6);
    }
}
