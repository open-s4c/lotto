use std::{
    ffi::CStr,
    fmt::{Debug, Display},
    hash::Hash,
    mem::MaybeUninit,
};

use crate::wrap;
use bincode::{de::read::Reader, enc::write::Writer};
use lotto_sys as raw;

pub type StableAddressMethod = raw::stable_address_method_t;

wrap!(StableAddress, raw::stable_address_t);

wrap!(MapAddress, raw::map_address_t);

impl Hash for StableAddress {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        // FIXME: This probably can be improved.
        let s = self.to_string();
        s.hash(state);
    }
}

impl Eq for StableAddress {}

impl StableAddress {
    /// Obtain a stable address for `addr` using the global sequencer
    /// config.
    pub fn with_default_method(addr: usize) -> StableAddress {
        let method = unsafe { (*raw::sequencer_config()).stable_address_method };
        StableAddress::with_method(addr, method)
    }

    /// Obtain a stable address for `addr` with a specified method.
    pub fn with_method(addr: usize, method: StableAddressMethod) -> StableAddress {
        StableAddress {
            inner: unsafe { raw::stable_address_get(addr, method) },
        }
    }

    /// Obtain a [`MapAddress`] reference.
    ///
    /// Only available when `LOTTO_STABLE_ADDRESS_MAP` is supported.
    #[cfg(feature = "stable_address_map")]
    pub fn as_map_address(&self) -> &MapAddress {
        assert!(self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP);
        unsafe { MapAddress::wrap(&self.inner.value.map) }
    }

    /// Obtain a mutable [`MapAddress`] reference.
    #[cfg(feature = "stable_address_map")]
    pub fn as_map_address_mut(&mut self) -> &mut MapAddress {
        assert!(self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP);
        unsafe { MapAddress::wrap_mut(&mut self.inner.value.map) }
    }
}

impl Display for StableAddress {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut buffer = [0u8; raw::STABLE_ADDRESS_MAX_LEN as usize];
        unsafe {
            raw::stable_address_sprint(self.as_ptr(), buffer.as_mut_ptr() as *mut i8);
        }
        let cstr = CStr::from_bytes_until_nul(&buffer).unwrap();
        let str = cstr.to_str().expect("valid utf-8");
        write!(f, "{}", str)
    }
}

impl Debug for StableAddress {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self)
    }
}

impl Clone for StableAddress {
    fn clone(&self) -> Self {
        let result = std::mem::MaybeUninit::<StableAddress>::uninit();
        unsafe {
            std::ptr::copy(
                self.as_ptr(),
                result.as_ptr() as *mut raw::stable_address_t,
                1,
            );
            result.assume_init()
        }
    }
}

impl PartialEq for StableAddress {
    fn eq(&self, other: &Self) -> bool {
        unsafe { raw::stable_address_equals(self.as_ptr(), other.as_ptr()) }
    }
}

impl bincode::Encode for StableAddress {
    fn encode<E: bincode::enc::Encoder>(
        &self,
        encoder: &mut E,
    ) -> Result<(), bincode::error::EncodeError> {
        const LEN: usize = std::mem::size_of::<raw::stable_address_t>();
        let mut bytes = [0u8; LEN];
        unsafe {
            std::ptr::copy(
                self.as_ptr(),
                bytes.as_mut_ptr() as *mut raw::stable_address_t,
                1,
            );
        }
        encoder.writer().write(&bytes)
    }
}

impl bincode::Decode for StableAddress {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        const LEN: usize = std::mem::size_of::<raw::stable_address_t>();
        let mut bytes = [0u8; LEN];
        decoder.reader().read(&mut bytes)?;
        let mut result = MaybeUninit::<Self>::uninit();
        unsafe {
            std::ptr::copy(
                bytes.as_ptr() as *const raw::stable_address_t,
                result.as_mut_ptr() as *mut raw::stable_address_t,
                1,
            );
            Ok(result.assume_init())
        }
    }
}

impl<'de> bincode::BorrowDecode<'de> for StableAddress {
    fn borrow_decode<D: bincode::de::BorrowDecoder<'de>>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let result = <StableAddress as bincode::Decode>::decode(decoder)?;
        Ok(result)
    }
}

impl MapAddress {
    pub fn name(&self) -> &CStr {
        // Safety: this pointer must be non-null, and is a valid C
        // string.
        unsafe { CStr::from_ptr(&self.inner.name as *const i8) }
    }

    pub fn offset(&self) -> u64 {
        self.offset
    }
}

#[cfg(test)]
mod tests {
    extern crate lotto_link;
    use super::*;
    use crate::brokers::statemgr::Serializable;

    #[cfg(feature = "stable_address_map")]
    static STATIC_VAR: u64 = 0;

    #[test]
    fn mask_marshable() {
        let x: i32 = 0; // Stack variable
        let sa = StableAddress::with_method(
            &x as *const _ as usize,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MASK,
        );
        const LEN: usize = size_of::<StableAddress>();
        let mut buf = [0u8; LEN];
        let marshal_size = sa.size();
        assert_eq!(marshal_size, LEN);
        let len = sa.marshal(&mut buf as *mut u8);
        assert_eq!(len, LEN);

        let mut sa2 =
            StableAddress::with_method(0, StableAddressMethod::STABLE_ADDRESS_METHOD_MASK);
        sa2.unmarshal(&buf as *const u8);
        assert_eq!(sa, sa2);
    }

    #[test]
    #[cfg(feature = "stable_address_map")]
    fn map_marshale() {
        let sa = StableAddress::with_method(
            &STATIC_VAR as *const _ as usize,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MASK,
        );
        const LEN: usize = size_of::<StableAddress>();
        let mut buf = [0u8; LEN];
        let marshal_size = sa.size();
        assert_eq!(marshal_size, LEN);
        let len = sa.marshal(&mut buf as *mut u8);
        assert_eq!(len, LEN);

        let mut sa2 =
            StableAddress::with_method(0, StableAddressMethod::STABLE_ADDRESS_METHOD_MASK);
        sa2.unmarshal(&buf as *const u8);
        assert_eq!(sa, sa2);
    }

    #[test]
    #[cfg(feature = "stable_address_map")]
    fn clone() {
        let sa = StableAddress::with_method(
            &STATIC_VAR as *const _ as usize,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MAP,
        );
        let sa2 = sa.clone();
        assert_eq!(sa, sa2);
    }
}
