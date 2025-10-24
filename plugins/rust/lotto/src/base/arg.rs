use lotto_sys as raw;

#[derive(Debug)]
pub enum HandlerArg {
    None,
    U8(u8),
    U16(u16),
    U32(u32),
    U64(u64),
    U128(u128),
    Addr(usize),
}

impl HandlerArg {
    /// Get the address.
    ///
    /// # Panics
    ///
    /// Panic if it's not `HandlerArg::Addr`.
    pub fn addr(&self) -> usize {
        if let HandlerArg::Addr(addr) = self {
            *addr
        } else {
            panic!(
                "Cannot get address out of a non-address HandlerArg {:?}",
                self
            );
        }
    }

    /// Get the u64 value.
    pub fn u64(&self) -> u64 {
        if let HandlerArg::U64(val) = self {
            *val
        } else {
            panic!("Cannot get u64 out of a non-u64 HandlerArg {:?}", self);
        }
    }
}

impl From<raw::arg_t> for HandlerArg {
    fn from(value: raw::arg_t) -> Self {
        match value.width {
            v if v == raw::arg_arg_width_ARG_EMPTY => HandlerArg::None,
            v if v == raw::arg_arg_width_ARG_U8 => HandlerArg::U8(unsafe { value.value.u8_ }),
            v if v == raw::arg_arg_width_ARG_U16 => HandlerArg::U16(unsafe { value.value.u16_ }),
            v if v == raw::arg_arg_width_ARG_U32 => HandlerArg::U32(unsafe { value.value.u32_ }),
            v if v == raw::arg_arg_width_ARG_U64 => HandlerArg::U64(unsafe { value.value.u64_ }),
            v if v == raw::arg_arg_width_ARG_PTR => HandlerArg::Addr(unsafe { value.value.ptr }),
            v if v == raw::arg_arg_width_ARG_U128 => {
                let val: u128 = unsafe { u128::from_ne_bytes(value.value.u128_.bytes) };
                HandlerArg::U128(val)
            }
            _ => panic!("Unknown `width` variant {:?} for `arg_t`", value.width),
        }
    }
}
