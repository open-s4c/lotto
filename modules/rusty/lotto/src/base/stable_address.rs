use std::{
    ffi::CStr,
    fmt::{Debug, Display},
    hash::Hash,
    mem::{offset_of, size_of},
    sync::Arc,
};

use bincode::{de::read::Reader, enc::write::Writer};
use lotto_sys as raw;

pub type StableAddressMethod = raw::stable_address_method_t;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord)]
pub enum StableAddress {
    Ptr(usize),
    Map(MapAddress),
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct MapAddress {
    name: Arc<CStr>,
    pub offset: u64,
}

// Keep the native trace layout, without carrying that layout in every event.
const WIRE_LEN: usize = size_of::<raw::stable_address_t>();
const TAG: usize = offset_of!(raw::stable_address_t, type_);
const VALUE: usize = offset_of!(raw::stable_address_t, value);
const NAME: usize = VALUE + offset_of!(raw::map_address_t, name);
const OFFSET: usize = VALUE + offset_of!(raw::map_address_t, offset);
const NAME_LEN: usize = unsafe { std::mem::zeroed::<raw::map_address_t>() }
    .name
    .len();

impl Hash for StableAddress {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        match self {
            Self::Ptr(ptr) => {
                raw::stable_address_stable_address_type_ADDRESS_PTR.hash(state);
                ptr.hash(state);
            }
            Self::Map(map) => {
                raw::stable_address_stable_address_type_ADDRESS_MAP.hash(state);
                map.name().to_bytes().hash(state);
                map.offset.hash(state);
            }
        }
    }
}

impl StableAddress {
    pub fn with_default_method(addr: usize) -> Self {
        let method = unsafe { (*raw::sequencer_config()).stable_address_method };
        Self::with_method(addr, method)
    }

    pub fn with_method(addr: usize, method: StableAddressMethod) -> Self {
        let address = unsafe { raw::stable_address_get(addr, method) };
        match address.type_ {
            raw::stable_address_stable_address_type_ADDRESS_PTR => {
                Self::Ptr(unsafe { address.value.ptr })
            }
            raw::stable_address_stable_address_type_ADDRESS_MAP => {
                let map = unsafe { &address.value.map };
                // Native map resolution produces a terminated filename.
                let name = unsafe { CStr::from_ptr(map.name.as_ptr()) };
                Self::Map(MapAddress {
                    name: Arc::from(name),
                    offset: map.offset,
                })
            }
            _ => unreachable!("Unknown stable address type"),
        }
    }

    pub fn as_map_address(&self) -> &MapAddress {
        match self {
            Self::Map(map) => map,
            _ => panic!("Expected a mapped address"),
        }
    }

    pub fn as_map_address_mut(&mut self) -> &mut MapAddress {
        match self {
            Self::Map(map) => map,
            _ => panic!("Expected a mapped address"),
        }
    }
}

impl Display for StableAddress {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Ptr(ptr) => write!(f, "0x{ptr:016x}"),
            Self::Map(map) => write!(f, "{}:0x{:016x}", map.name.to_string_lossy(), map.offset),
        }
    }
}

impl Debug for StableAddress {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        Display::fmt(self, f)
    }
}

impl bincode::Encode for StableAddress {
    fn encode<E: bincode::enc::Encoder>(
        &self,
        encoder: &mut E,
    ) -> Result<(), bincode::error::EncodeError> {
        // Zero padding and unused union bytes explicitly, rather than reading
        // potentially uninitialized padding from a C struct.
        let mut bytes = [0u8; WIRE_LEN];
        let tag = match self {
            Self::Ptr(ptr) => {
                bytes[VALUE..VALUE + size_of::<usize>()].copy_from_slice(&ptr.to_ne_bytes());
                raw::stable_address_stable_address_type_ADDRESS_PTR
            }
            Self::Map(map) => {
                let name = map.name.to_bytes_with_nul();
                bytes[NAME..NAME + name.len()].copy_from_slice(name);
                bytes[OFFSET..OFFSET + 8].copy_from_slice(&map.offset.to_ne_bytes());
                raw::stable_address_stable_address_type_ADDRESS_MAP
            }
        };
        bytes[TAG..TAG + size_of_val(&tag)].copy_from_slice(&tag.to_ne_bytes());
        encoder.writer().write(&bytes)
    }
}

impl bincode::Decode for StableAddress {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let mut bytes = [0u8; WIRE_LEN];
        decoder.reader().read(&mut bytes)?;
        let tag = u32::from_ne_bytes(bytes[TAG..TAG + 4].try_into().unwrap());
        match tag {
            raw::stable_address_stable_address_type_ADDRESS_PTR => Ok(Self::Ptr(
                usize::from_ne_bytes(bytes[VALUE..VALUE + size_of::<usize>()].try_into().unwrap()),
            )),
            raw::stable_address_stable_address_type_ADDRESS_MAP => {
                let name =
                    CStr::from_bytes_until_nul(&bytes[NAME..NAME + NAME_LEN]).map_err(|_| {
                        bincode::error::DecodeError::Other("Unterminated mapped address name")
                    })?;
                Ok(Self::Map(MapAddress {
                    name: Arc::from(name),
                    offset: u64::from_ne_bytes(bytes[OFFSET..OFFSET + 8].try_into().unwrap()),
                }))
            }
            _ => Err(bincode::error::DecodeError::Other(
                "Unknown stable address type",
            )),
        }
    }
}

impl<'de> bincode::BorrowDecode<'de> for StableAddress {
    fn borrow_decode<D: bincode::de::BorrowDecoder<'de>>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        <Self as bincode::Decode>::decode(decoder)
    }
}

impl MapAddress {
    pub fn name(&self) -> &CStr {
        &self.name
    }
    pub fn offset(&self) -> u64 {
        self.offset
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::hash_map::DefaultHasher;
    use std::hash::Hasher;

    #[test]
    fn compact_addresses_preserve_native_trace_layout() {
        assert!(size_of::<StableAddress>() <= 32);
        let pc = compact_addresses_preserve_native_trace_layout as *const () as usize;
        for method in [
            StableAddressMethod::STABLE_ADDRESS_METHOD_NONE,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MASK,
            StableAddressMethod::STABLE_ADDRESS_METHOD_MAP,
        ] {
            let address = StableAddress::with_method(pc, method);
            let config = bincode::config::standard();
            let bytes = bincode::encode_to_vec(&address, config).unwrap();
            assert_eq!(bytes.len(), WIRE_LEN);
            let native = unsafe { raw::stable_address_get(pc, method) };
            let encoded =
                unsafe { std::ptr::read_unaligned(bytes.as_ptr().cast::<raw::stable_address_t>()) };
            assert!(unsafe { raw::stable_address_equals(&native, &encoded) });
            let (decoded, used): (StableAddress, _) =
                bincode::decode_from_slice(&bytes, config).unwrap();
            assert_eq!(used, WIRE_LEN);
            assert_eq!(decoded, address);
            let cloned = address.clone();
            if let (StableAddress::Map(a), StableAddress::Map(b)) = (&address, &cloned) {
                assert!(Arc::ptr_eq(&a.name, &b.name));
            }
        }
    }

    #[test]
    fn full_length_names_and_clones() {
        let mut name = vec![b'x'; NAME_LEN];
        name[NAME_LEN - 1] = 0;
        let a = StableAddress::Map(MapAddress {
            name: Arc::from(CStr::from_bytes_with_nul(&name).unwrap()),
            offset: 17,
        });
        let mut b = a.clone();
        b.as_map_address_mut().offset = 18;
        assert_eq!(a.as_map_address().offset(), 17);
        assert_ne!(a, b);
        assert!(Arc::ptr_eq(
            &a.as_map_address().name,
            &b.as_map_address().name
        ));
        let config = bincode::config::standard();
        let bytes = bincode::encode_to_vec(&a, config).unwrap();
        let (decoded, _): (StableAddress, _) = bincode::decode_from_slice(&bytes, config).unwrap();
        assert_eq!(a, decoded);
    }

    #[test]
    fn legacy_unused_bytes_and_non_utf8_names() {
        let mut bytes = [0u8; WIRE_LEN];
        bytes[TAG..TAG + 4]
            .copy_from_slice(&raw::stable_address_stable_address_type_ADDRESS_MAP.to_ne_bytes());
        bytes[NAME] = 0xff;
        bytes[OFFSET..OFFSET + 8].copy_from_slice(&42u64.to_ne_bytes());
        let decode = |bytes: &[u8]| {
            bincode::decode_from_slice::<StableAddress, _>(bytes, bincode::config::standard())
                .unwrap()
                .0
        };
        let a = decode(&bytes);
        bytes[NAME + 100] = 1;
        let b = decode(&bytes);
        assert_eq!(a, b);
        assert_eq!(a.cmp(&b), std::cmp::Ordering::Equal);
        let hash = |a: &StableAddress| {
            let mut h = DefaultHasher::new();
            a.hash(&mut h);
            h.finish()
        };
        assert_eq!(hash(&a), hash(&b));
        assert_eq!(a.as_map_address().name().to_bytes(), &[0xff]);
        // Reject malformed input before constructing an owned address.
        bytes[NAME..NAME + NAME_LEN].fill(1);
        assert!(bincode::decode_from_slice::<StableAddress, _>(
            &bytes,
            bincode::config::standard()
        )
        .is_err());
        bytes[TAG..TAG + 4].copy_from_slice(&99u32.to_ne_bytes());
        assert!(bincode::decode_from_slice::<StableAddress, _>(
            &bytes,
            bincode::config::standard()
        )
        .is_err());
    }
}
