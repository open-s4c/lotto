use bincode::{Decode, Encode};

pub use crate::vaddr::VAddr;

/// Information about an operation that modifies a memory address.
///
/// Note: Corresponds to `modify_t` in the C code.
#[derive(Debug, Clone, Hash, PartialEq, Eq)]
pub struct Modify {
    /// Size of the modified memory region.
    pub size: u8,

    /// Real address.
    ///
    /// Only usable during the same execution!
    pub real_addr: u64,

    /// Modified address.
    pub addr: VAddr,

    /// Kind of the modification
    pub kind: ModifyKind,
}

/// The kind of an operation that modifies a memory address.
///
/// NOTE: Corresponds to `modify_t` in the C code.
#[derive(Encode, Decode, Debug, Clone, Hash, PartialEq, Eq)]
pub enum ModifyKind {
    /// Atomic write.
    AWrite {
        /// The value to be written.
        value: u64,
    },

    /// Plain write.
    Write,

    /// Fetch-and-add etc.
    Rmw {
        /// The delta value.
        value: u64,
    },

    /// Compare and swap
    Cas {
        /// Expected current value.
        expected: u64,

        /// The new value to be put in the memory address.
        new: u64,
    },

    /// Non-atomic Xchg
    Xchg {
        /// The new value.
        value: u64,
    },
}

impl Modify {
    /// Returns whether the two modifications are on the same address.
    pub fn conflicting_with(&self, other: &Modify) -> bool {
        // FIXME: The case of Overlapping regions
        if self.addr == other.addr {
            return true;
        }
        false
    }

    /// Predict whether a CAS operation will succeed or not.
    ///
    /// # Panics
    ///
    /// Panic if (1) `self` is not a CAS operation, or (2) the size of
    /// the modified memory is unrecognized, or (3) `real_addr` is
    /// invalid.
    pub fn predict_cas(&self) -> bool {
        let expected = match self.kind {
            ModifyKind::Cas { expected, .. } => expected,
            _ => {
                panic!("Trying to predict a non-CAS operation")
            }
        };
        if self.real_addr == PLACEHOLDER {
            panic!("real_addr is PLACEHOLDER! Modify is {:#?}", self);
        }
        match self.size {
            1 => unsafe { *(self.real_addr as *const u8) as u64 == expected },
            2 => unsafe { *(self.real_addr as *const u16) as u64 == expected },
            4 => unsafe { *(self.real_addr as *const u32) as u64 == expected },
            8 => unsafe { *(self.real_addr as *const u64) as u64 == expected },
            _ => panic!("Unknown size of pointee"),
        }
    }

    /// Return if the `real_addr` is invalid. I.e., not from the
    /// current execution.
    pub fn has_invalid_real_addr(&self) -> bool {
        self.real_addr == PLACEHOLDER
    }
}

#[cfg(test)]
mod tests {
    use std::mem;

    use super::*;
    use lotto::{brokers::statemgr::Serializable, raw};

    static X: u64 = 123;
    static Y: u64 = 321;
    const SIZE: u8 = std::mem::size_of::<u64>() as u8;

    fn ctx() -> raw::context_t {
        raw::context_t {
            pc: 0xdeadbeef,
            func: c"UNKNOWN".as_ptr(),
            func_addr: 0xf114514f,
            ..unsafe { std::mem::zeroed() }
        }
    }

    fn x_va() -> VAddr {
        VAddr::get(&ctx(), &X as *const _ as usize)
    }

    fn y_va() -> VAddr {
        VAddr::get(&ctx(), &Y as *const _ as usize)
    }

    #[test]
    fn conflict_write_cas() {
        let x = x_va();
        let write = Modify {
            addr: x.clone(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::AWrite { value: 0xf114514f },
        };
        let cas = Modify {
            addr: x.clone(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::Cas {
                expected: 23333,
                new: 344444,
            },
        };
        assert!(write.conflicting_with(&cas));
    }

    #[test]
    fn nonconflict() {
        let x = x_va();
        let y = y_va();
        let write_x = Modify {
            addr: x.clone(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::AWrite { value: 0xf114514f },
        };
        let write_y = Modify {
            addr: y.clone(),
            real_addr: &Y as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::Cas {
                expected: 23333,
                new: 344444,
            },
        };
        assert!(!write_x.conflicting_with(&write_y));
    }

    #[test]
    fn serialization() {
        let before = Modify {
            addr: x_va(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::AWrite { value: 0xf114514f },
        };
        let mut buf = vec![0; before.size()];
        before.marshal(buf.as_mut_ptr());
        let mut after: Modify = unsafe { mem::zeroed() };
        after.unmarshal(buf.as_ptr());
        assert_eq!(after.size, before.size);
        assert_eq!(after.addr, before.addr);
        assert_eq!(after.kind, before.kind);
        // any comparison between after.real_addr and before.real_addr
        // is meaningless. so we replace it as a placeholder value.
        assert_eq!(after.real_addr, PLACEHOLDER);
    }
}

const PLACEHOLDER: u64 = 0x01140514;

impl bincode::Encode for Modify {
    fn encode<E: bincode::enc::Encoder>(
        &self,
        encoder: &mut E,
    ) -> Result<(), bincode::error::EncodeError> {
        self.size.encode(encoder)?;
        self.addr.encode(encoder)?;
        self.kind.encode(encoder)?;
        Ok(())
    }
}

impl bincode::Decode for Modify {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let size = u8::decode(decoder)?;
        let addr = VAddr::decode(decoder)?;
        let kind = ModifyKind::decode(decoder)?;
        Ok(Self {
            size,
            addr,
            real_addr: PLACEHOLDER,
            kind,
        })
    }
}

impl<'de> bincode::BorrowDecode<'de> for Modify {
    fn borrow_decode<D: bincode::de::BorrowDecoder<'de>>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        Self::decode(decoder)
    }
}
