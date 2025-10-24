#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused)]
#![allow(clippy::missing_safety_doc)]
//#![allow(unnecessary_transmutes)]

use std::ffi::CStr;

use bincode;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

impl std::fmt::Display for base_category {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let ptr = unsafe { category_str(*self) };
        let cstr = unsafe { CStr::from_ptr(ptr) };
        let s = cstr.to_str().expect("valid utf-8");
        write!(f, "{}", s)
    }
}

impl bincode::Encode for base_category {
    fn encode<E: bincode::enc::Encoder>(
        &self,
        encoder: &mut E,
    ) -> Result<(), bincode::error::EncodeError> {
        bincode::Encode::encode(&self.0, encoder)
    }
}

impl bincode::Decode for base_category {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let val = std::ffi::c_uint::decode(decoder)?;
        Ok(base_category(val))
    }
}

impl<'de> bincode::BorrowDecode<'de> for base_category {
    fn borrow_decode<D: bincode::de::BorrowDecoder<'de>>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let val = std::ffi::c_uint::borrow_decode(decoder)?;
        Ok(base_category(val))
    }
}

impl reason {
    /// Whether the execution terminated due to Lotto runtime
    /// situations.
    pub fn is_runtime(self) -> bool {
        unsafe { reason_is_runtime(self) }
    }

    pub fn is_shutdown(self) -> bool {
        unsafe { reason_is_shutdown(self) }
    }

    pub fn is_abort(self) -> bool {
        unsafe { reason_is_abort(self) }
    }

    pub fn is_terminate(self) -> bool {
        unsafe { reason_is_terminate(self) }
    }

    pub fn is_record_final(self) -> bool {
        unsafe { reason_is_record_final(self) }
    }
}

impl std::fmt::Display for record {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.0 == 0 {
            return write!(f, "NONE");
        }
        const FLAGS: [(u32, &str); 8] = [
            (record::RECORD_INFO.0, "INFO"),
            (record::RECORD_SCHED.0, "SCHED"),
            (record::RECORD_START.0, "START"),
            (record::RECORD_EXIT.0, "EXIT"),
            (record::RECORD_CONFIG.0, "CONFIG"),
            (record::RECORD_SHUTDOWN_LOCK.0, "SHUTDOWN_LOCK"),
            (record::RECORD_OPAQUE.0, "OPAQUE"),
            (record::RECORD_FORCE.0, "FORCE"),
        ];
        const _: () = assert!(record::_RECORD_LAST.0 == 0x80);
        let mut first = true;
        for &(flag, name) in &FLAGS {
            if (self.0 & flag) != 0 {
                if !first {
                    write!(f, "|")?;
                }
                write!(f, "{}", name)?;
                first = false;
            }
        }
        Ok(())
    }
}
