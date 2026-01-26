//! # Memory access
//!
//! Lotto the engine only records "how" something happened (e.g., it
//! is an instruction whose PC is 0x1234 executed by task 1), but not
//! "what" actually changed, and the semantics of the program. This
//! information is not adequate for comparing events across consistent
//! executions, and must be enriched with more information. This
//! module provides types and data structures to support more
//! fine-grained information about memory operations, i.e. semantics
//! of memory operations in the framework of a formal memory model.
//!
//! Note, in the implementation, all memory-modifying operations are
//! collectively called Modify, and all memory-reading operations,
//! excluding those also modify memory, Read.
//!
//! ## Relations
//!
//! The ultimate goal of all of these kerfuffle is to establish a few
//! important relations:
//!
//! 1. `W` reads-from `R`: A read operation `R` reads the value
//!    written by a write operation `W`. Note `W` is the source.
//!
//! 2. `W1` modification-order `W2`: (on a memory location) the
//!    modification by `W1` is immediately before `W2`.
//!
//! 3. `R1` reads-before `W2`: `R1` reads a value which is the
//!    previous version before `W2`.
//!
//!    Or, formally: `reads-before = rev(reads-from); mo` and
//!    excluding all self cycles (to prevent RMW from reading-before
//!    itself).
//!
//! Finally, `sequenced-before` (program order) is trivial in Lotto:
//! `A` sequenced-before `B` if and only if `A.id == B.id` and
//! `A.clk < B.clk` (modulo category).
//!
//! ## Properties and Simplifications
//!
//! We note the following properties:
//!
//! 1. Reads-from is functional in W: a read can only read from one
//!    write.
//!
//! 2. Modification-order is a total order.
//!
//! 3. Since reads-before is a composition of reads-from and
//!    modification-order, it can be left out.
//!
//! Therefore, they can be simplified as:
//!
//! 1. `r.readFromModify` reads-from `r`
//!
//! 2. `modify.next` modification-order `modify` modification-order `modify.next`

use bincode::{Decode, Encode};
use lotto::base::category::Category;

pub use crate::vaddr::VAddr;
use crate::{sized_read, Event, Transition};

/// A memory access operation.
#[derive(Debug, Decode, Encode, Clone, PartialEq, Eq, Hash)]
pub enum MemoryAccess {
    /// Plain reads.
    Read(Read),

    /// Write, RMW, or CAS.
    Modify(Modify),
}

impl MemoryAccess {
    pub fn as_read(&self) -> Option<&Read> {
        match self {
            MemoryAccess::Read(r) => Some(r),
            MemoryAccess::Modify(_) => None,
        }
    }

    pub fn as_modify(&self) -> Option<&Modify> {
        match self {
            MemoryAccess::Read(_) => None,
            MemoryAccess::Modify(m) => Some(m),
        }
    }

    pub fn loaded_value(&self) -> Option<u64> {
        match self {
            MemoryAccess::Read(r) => Some(r.value()),
            MemoryAccess::Modify(m) => m.loaded_value(),
        }
    }

    pub fn addr(&self) -> &VAddr {
        match self {
            MemoryAccess::Read(r) => &r.addr,
            MemoryAccess::Modify(m) => &m.addr,
        }
    }

    pub fn has_invalid_real_addr(&self) -> bool {
        match self {
            MemoryAccess::Read(r) => r.has_invalid_real_addr(),
            MemoryAccess::Modify(m) => m.has_invalid_real_addr(),
        }
    }

    pub fn to_modify(&self) -> Option<&Modify> {
        match self {
            MemoryAccess::Modify(m) => Some(m),
            _ => None,
        }
    }

    /// Returns true if self and other are modify's and they are
    /// conflicting with each other, i.e. they are racy.
    ///
    /// TODO(kai): check if reads should be incorporated.
    pub fn conflicting_with(&self, other: &MemoryAccess) -> bool {
        match (self, other) {
            (MemoryAccess::Modify(m1), MemoryAccess::Modify(m2)) => m1.conflicting_with(m2),
            _ => false,
        }
    }

    /// Returns true if the memory access is a CAS and is predicted to
    /// be successful.
    pub fn predict_cas(&self) -> bool {
        match self {
            MemoryAccess::Modify(m) => m.predict_cas(),
            _ => false,
        }
    }
}

/// Information about an operation that reads a memory location.
#[derive(Debug, Clone, Hash, PartialEq, Eq)]
pub struct Read {
    /// Size of the read.
    pub size: u8,

    /// Real-time address.
    ///
    /// Only usable during the same execution!
    pub real_addr: u64,

    /// Read address.
    pub addr: VAddr,

    /// Value
    pub value: u64,
}

impl Read {
    pub fn has_invalid_real_addr(&self) -> bool {
        self.real_addr == PLACEHOLDER
    }

    pub fn value(&self) -> u64 {
        self.value
    }

    #[allow(unreachable_code)]
    pub unsafe fn reload_value(&self) -> u64 {
        panic!("should not call it");

        // HACK: replay-rs can try to reload values...
        if !self.has_invalid_real_addr() {
            sized_read(self.real_addr as u64, self.size as usize)
        } else {
            self.value
        }
    }
}

/// Information about an operation that modifies a memory location.
#[derive(Debug, Clone, Hash, PartialEq, Eq)]
pub struct Modify {
    /// Size of the modified memory region.
    pub size: u8,

    /// Real-time address.
    ///
    /// Only usable during the same execution!
    pub real_addr: u64,

    /// Modified address.
    pub addr: VAddr,

    /// Kind of the modification
    pub kind: ModifyKind,

    /// Read Value
    ///
    /// CAS and RMW operations' effects are determined by the values
    /// they read.
    pub read_value: Option<u64>,
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

    /// RMW, CAS, and Xchg can only happen if they read some values.
    pub fn loaded_value(&self) -> Option<u64> {
        self.read_value
    }

    pub unsafe fn reload_value(&self) -> Option<u64> {
        if self.has_invalid_real_addr() {
            return self.loaded_value();
        }
        let new_val = sized_read(self.real_addr as u64, self.size as usize);
        Some(new_val)
    }
}

/// The kind of an operation that modifies a memory address.
///
/// NOTE: Corresponds to `modify_t` in the C code.
#[derive(Encode, Decode, Debug, Clone, Hash, PartialEq, Eq)]
pub enum ModifyKind {
    /// Atomic write.
    AWrite {
        is_after: bool,

        /// The value to be written.
        value: u64,
    },

    /// Plain write.
    Write,

    /// Fetch-and-add etc.
    Rmw {
        is_after: bool,

        /// The delta value.
        delta: u64,
    },

    /// Compare and swap
    Cas {
        is_after: bool,

        /// Expected current value.
        expected: u64,

        /// The new value to be put in the memory address.
        new: u64,
    },

    /// Xchg
    Xchg {
        is_after: bool,

        /// The new value.
        value: u64,
    },
}

impl ModifyKind {
    pub fn is_after(&self) -> bool {
        match self {
            Self::AWrite { is_after, .. } => *is_after,
            Self::Rmw { is_after, .. } => *is_after,
            Self::Xchg { is_after, .. } => *is_after,
            Self::Cas { is_after, .. } => *is_after,
            _ => false,
        }
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
            kind: ModifyKind::AWrite {
                is_after: false,
                value: 0xf114514f,
            },
            read_value: None,
        };
        let cas = Modify {
            addr: x.clone(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::Cas {
                is_after: false,
                expected: 23333,
                new: 344444,
            },
            read_value: None,
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
            kind: ModifyKind::AWrite {
                is_after: false,
                value: 0xf114514f,
            },
            read_value: None,
        };
        let write_y = Modify {
            addr: y.clone(),
            real_addr: &Y as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::Cas {
                is_after: false,
                expected: 23333,
                new: 344444,
            },
            read_value: None,
        };
        assert!(!write_x.conflicting_with(&write_y));
    }

    #[test]
    fn serialization() {
        let before = Modify {
            addr: x_va(),
            real_addr: &X as *const u64 as u64,
            size: SIZE,
            kind: ModifyKind::AWrite {
                is_after: false,
                value: 0xf114514f,
            },
            read_value: None,
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
        self.read_value.encode(encoder)?;
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
        let read_value = Option::<u64>::decode(decoder)?;
        Ok(Self {
            size,
            addr,
            real_addr: PLACEHOLDER,
            kind,
            read_value,
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

impl bincode::Encode for Read {
    fn encode<E: bincode::enc::Encoder>(
        &self,
        encoder: &mut E,
    ) -> Result<(), bincode::error::EncodeError> {
        self.size.encode(encoder)?;
        self.addr.encode(encoder)?;
        self.value.encode(encoder)?;
        Ok(())
    }
}

impl bincode::Decode for Read {
    fn decode<D: bincode::de::Decoder>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        let size = u8::decode(decoder)?;
        let addr = VAddr::decode(decoder)?;
        let value = u64::decode(decoder)?;
        Ok(Self {
            size,
            addr,
            real_addr: PLACEHOLDER,
            value,
        })
    }
}

impl<'de> bincode::BorrowDecode<'de> for Read {
    fn borrow_decode<D: bincode::de::BorrowDecoder<'de>>(
        decoder: &mut D,
    ) -> Result<Self, bincode::error::DecodeError> {
        Self::decode(decoder)
    }
}

/// Extra methods for memory-operation-like information, including
/// [`Category`].
///
/// All types implementing this trait should *not* concern themselves
/// with non-memory events, such as `TASK_INIT`, etc.
pub trait MemoryOperationExt {
    /// Check if this is a memory operation at all.
    ///
    /// You should *not* override the default implementation.
    fn is_memory_operation(&self) -> bool {
        self.is_read() || self.is_write()
    }

    /// This operation reads a memory location.
    ///
    /// This includes both `BEFORE` and `AFTER` events of CAS and
    /// RMW. In case they should be handled separately, use `is_cas`,
    /// etc.
    fn is_read(&self) -> bool;

    /// This operation writes to a memory location.
    ///
    /// Includes CAS and RMW.
    fn is_write(&self) -> bool;

    /// This operation is atomic.
    fn is_atomic(&self) -> bool;

    /// This operation is a `BEFORE` event.
    fn is_before(&self) -> bool;

    /// This operation is an `AFTER` event.
    fn is_after(&self) -> bool;

    /// This operation is a CAS event.
    fn is_cas(&self) -> bool;

    /// This operation is an RMW event.
    fn is_rmw(&self) -> bool;

    /// This operation is an Xchg event.
    fn is_xchg(&self) -> bool;
}

impl MemoryOperationExt for Category {
    fn is_read(&self) -> bool {
        matches!(
            *self,
            Category::CAT_BEFORE_READ
                | Category::CAT_BEFORE_AREAD
                | Category::CAT_AFTER_AREAD
                | Category::CAT_BEFORE_RMW
                | Category::CAT_AFTER_RMW
                | Category::CAT_BEFORE_CMPXCHG
                | Category::CAT_AFTER_CMPXCHG_S
                | Category::CAT_AFTER_CMPXCHG_F
                | Category::CAT_BEFORE_XCHG
                | Category::CAT_AFTER_XCHG
        )
    }

    fn is_write(&self) -> bool {
        matches!(
            *self,
            Category::CAT_BEFORE_WRITE
                | Category::CAT_BEFORE_AWRITE
                | Category::CAT_AFTER_AWRITE
                | Category::CAT_BEFORE_RMW
                | Category::CAT_AFTER_RMW
                | Category::CAT_BEFORE_CMPXCHG
                | Category::CAT_AFTER_CMPXCHG_S
                | Category::CAT_AFTER_CMPXCHG_F
                | Category::CAT_BEFORE_XCHG
                | Category::CAT_AFTER_XCHG
        )
    }

    fn is_atomic(&self) -> bool {
        matches!(
            *self,
            Category::CAT_BEFORE_AWRITE
                | Category::CAT_AFTER_AWRITE
                | Category::CAT_BEFORE_AREAD
                | Category::CAT_AFTER_AREAD
                | Category::CAT_BEFORE_RMW
                | Category::CAT_AFTER_RMW
                | Category::CAT_BEFORE_CMPXCHG
                | Category::CAT_AFTER_CMPXCHG_S
                | Category::CAT_AFTER_CMPXCHG_F
                | Category::CAT_BEFORE_XCHG
                | Category::CAT_AFTER_XCHG
        )
    }

    fn is_before(&self) -> bool {
        matches!(
            *self,
            Category::CAT_BEFORE_READ
                | Category::CAT_BEFORE_AREAD
                | Category::CAT_BEFORE_RMW
                | Category::CAT_BEFORE_CMPXCHG
                | Category::CAT_BEFORE_XCHG
                | Category::CAT_BEFORE_WRITE
                | Category::CAT_BEFORE_AWRITE
        )
    }

    fn is_after(&self) -> bool {
        matches!(
            *self,
            Category::CAT_AFTER_AREAD
                | Category::CAT_AFTER_RMW
                | Category::CAT_AFTER_CMPXCHG_S
                | Category::CAT_AFTER_CMPXCHG_F
                | Category::CAT_AFTER_XCHG
                | Category::CAT_AFTER_AWRITE
        )
    }

    fn is_cas(&self) -> bool {
        matches!(
            *self,
            Category::CAT_BEFORE_CMPXCHG
                | Category::CAT_AFTER_CMPXCHG_S
                | Category::CAT_AFTER_CMPXCHG_F
        )
    }

    fn is_rmw(&self) -> bool {
        matches!(*self, Category::CAT_BEFORE_RMW | Category::CAT_AFTER_RMW)
    }

    fn is_xchg(&self) -> bool {
        matches!(*self, Category::CAT_BEFORE_XCHG | Category::CAT_AFTER_XCHG)
    }
}

macro_rules! impl_MemoryOperationExt_delegate {
    ($ty:ty, $($fld:tt)*) => {
        impl MemoryOperationExt for $ty {
            fn is_memory_operation(&self) -> bool { self.$($fld)*.is_memory_operation() }
            fn is_read(&self) -> bool   { self.$($fld)*.is_read() }
            fn is_write(&self) -> bool  { self.$($fld)*.is_write() }
            fn is_atomic(&self) -> bool { self.$($fld)*.is_atomic() }
            fn is_before(&self) -> bool { self.$($fld)*.is_before() }
            fn is_after(&self) -> bool  { self.$($fld)*.is_after() }
            fn is_cas(&self) -> bool    { self.$($fld)*.is_cas() }
            fn is_rmw(&self) -> bool    { self.$($fld)*.is_rmw() }
            fn is_xchg(&self) -> bool   { self.$($fld)*.is_xchg() }
        }
    }
}

impl_MemoryOperationExt_delegate!(Transition, cat);
impl_MemoryOperationExt_delegate!(Event, t);
