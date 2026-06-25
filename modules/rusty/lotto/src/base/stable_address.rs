use std::{
    ffi::{CStr, CString},
    fmt::{Debug, Display},
    hash::Hash,
    os::raw::c_char,
};

use crate::wrap;
use lotto_sys as raw;

pub type StableAddressMethod = raw::stable_address_method_t;

wrap!(StableAddress, raw::stable_address_t);

wrap!(MapAddress, raw::map_address_t);

impl Hash for StableAddress {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.inner.type_.hash(state);
        if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_PTR {
            unsafe { self.inner.value.ptr }.hash(state);
        } else if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP {
            let map = unsafe { &self.inner.value.map };
            let name = unsafe { CStr::from_ptr(map.name.as_ptr()) };
            name.to_bytes().hash(state);
            map.offset.hash(state);
        } else {
            unreachable!("Unknown stable_address_method");
        }
    }
}

impl Eq for StableAddress {}

impl PartialOrd for StableAddress {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for StableAddress {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        assert!(
            self.inner.type_ == other.inner.type_,
            "Incompatible stable_address_method"
        );
        if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_PTR {
            unsafe { self.inner.value.ptr.cmp(&other.inner.value.ptr) }
        } else if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP {
            let this = unsafe { MapAddress::wrap(&self.inner.value.map) };
            let that = unsafe { MapAddress::wrap(&other.inner.value.map) };
            (&this.name, this.offset).cmp(&(&that.name, that.offset))
        } else {
            unreachable!("Unknown stable_address_method");
        }
    }
}

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
    pub fn as_map_address(&self) -> &MapAddress {
        assert!(self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP);
        unsafe { MapAddress::wrap(&self.inner.value.map) }
    }

    /// Obtain a mutable [`MapAddress`] reference.
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
        self.inner.type_.encode(encoder)?;
        if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_PTR {
            unsafe { self.inner.value.ptr }.encode(encoder)
        } else if self.inner.type_ == raw::stable_address_stable_address_type_ADDRESS_MAP {
            let map = unsafe { &self.inner.value.map };
            let name = unsafe { CStr::from_ptr(map.name.as_ptr()) }
                .to_str()
                .map_err(|_| bincode::error::EncodeError::Other("invalid map address path"))?;
            name.encode(encoder)?;
            map.offset.encode(encoder)
        } else {
            Err(bincode::error::EncodeError::Other(
                "unknown stable address type",
            ))
        }
    }
}

impl bincode::Decode for StableAddress {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let type_ = raw::stable_address_stable_address_type::decode(decoder)?;
        if type_ == raw::stable_address_stable_address_type_ADDRESS_PTR {
            let ptr = usize::decode(decoder)?;
            Ok(StableAddress {
                inner: raw::stable_address {
                    type_,
                    value: raw::stable_address_stable_address_value { ptr },
                },
            })
        } else if type_ == raw::stable_address_stable_address_type_ADDRESS_MAP {
            let name = String::decode(decoder)?;
            let offset = u64::decode(decoder)?;
            let c_name = CString::new(name)
                .map_err(|_| bincode::error::DecodeError::Other("invalid map address path"))?;
            let mut map = raw::map_address_t {
                name: [0 as c_char; std::mem::size_of::<raw::map_address_t>() - 8],
                offset,
            };
            let bytes = c_name.as_bytes_with_nul();
            if bytes.len() > map.name.len() {
                return Err(bincode::error::DecodeError::Other(
                    "map address path too long",
                ));
            }
            unsafe {
                std::ptr::copy_nonoverlapping(
                    bytes.as_ptr() as *const c_char,
                    map.name.as_mut_ptr(),
                    bytes.len(),
                );
            }
            Ok(StableAddress {
                inner: raw::stable_address {
                    type_,
                    value: raw::stable_address_stable_address_value { map },
                },
            })
        } else {
            Err(bincode::error::DecodeError::Other(
                "unknown stable address type",
            ))
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
    fn clone() {
        let sa = StableAddress::with_method(
            &STATIC_VAR as *const _ as usize,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MAP,
        );
        let sa2 = sa.clone();
        assert_eq!(sa, sa2);
    }
}
