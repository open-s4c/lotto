use std::ffi::CStr;

use lotto::base::StableAddress;
use lotto::brokers::{Decode, Encode};
use lotto::raw;

/// The stabilized address of a stack variable.
#[derive(Clone, Eq, Debug, Encode, Decode, Hash)]
pub struct StableStackAddress {
    /// Global function symbol
    func: String,

    /// Return address for distinguishing call sites
    ra: StableAddress,

    /// Base address of the stack frame
    frame_base: usize,

    /// Address of the stack variable, offset by func return address
    offset: isize,
}

impl PartialEq for StableStackAddress {
    fn eq(&self, other: &Self) -> bool {
        // FIXME: To handle recursion we still need a counter somehow
        return self.func == other.func && self.offset == other.offset && self.ra == other.ra;
    }
}

impl StableStackAddress {
    pub fn offset(&self, delta: isize) -> StableStackAddress {
        StableStackAddress {
            func: self.func.clone(),
            ra: self.ra.clone(),
            frame_base: self.frame_base,
            offset: self.offset + delta,
        }
    }
}

/// Variable address.
#[derive(Clone, PartialEq, Eq, Debug, Encode, Decode, Hash)]
pub enum VAddr {
    Stack(StableStackAddress),
    Global(StableAddress),
}

impl VAddr {
    /// Obtain a [`VAddr`] from a capture point.
    ///
    /// Global variables are stablized using the default stable
    /// address method.
    ///
    /// NOTE: This only reliably works when it's called from the same
    /// thread to whose stack this variable belongs.
    pub fn get(ctx: &raw::context_t, addr: usize) -> VAddr {
        let (stack_start, stack_end) = get_stack_range();
        if addr >= stack_start as usize && addr < stack_end as usize {
            VAddr::Stack(StableStackAddress {
                func: unsafe { CStr::from_ptr(ctx.func).to_str().unwrap().to_string() },
                ra: StableAddress::with_default_method(ctx.pc),
                frame_base: ctx.func_addr,
                offset: (addr as isize) - (ctx.func_addr as isize),
            })
        } else {
            VAddr::Global(StableAddress::with_default_method(addr))
        }
    }
}

/// Obtain the current memory range of the stack.
pub fn get_stack_range() -> (u64, u64) {
    unsafe {
        let mut attr: libc::pthread_attr_t = std::mem::zeroed();
        let mut stack_addr: *mut libc::c_void = std::ptr::null_mut();
        let mut stack_size: libc::size_t = 0;

        libc::pthread_getattr_np(libc::pthread_self(), &mut attr);
        libc::pthread_attr_getstack(&attr, &mut stack_addr, &mut stack_size);

        let start = stack_addr as u64;
        let end = start + stack_size as u64;

        (start, end)
    }
}

#[cfg(test)]
mod tests {
    extern crate lotto_link;
    use super::*;
    use lotto::brokers::statemgr::Serializable;

    fn fake_ctx() -> raw::context_t {
        raw::context_t {
            pc: 0xdeadbeef,
            func: c"UNKNOWN".as_ptr(),
            func_addr: 0xf114514f,
            ..unsafe { std::mem::zeroed() }
        }
    }

    #[test]
    fn stack_var_in_range() {
        let x = 123;
        let (a, b) = get_stack_range();
        let ptr = &x as *const _ as u64;
        assert!(ptr >= a && ptr < b);
    }

    #[test]
    fn stack_var() {
        let x = 123;
        let va = VAddr::get(&fake_ctx(), &x as *const _ as usize);
        assert!(matches!(va, VAddr::Stack(_)));
    }

    #[test]
    fn global_var() {
        static X: i32 = 321;
        let va = VAddr::get(&fake_ctx(), &X as *const _ as usize);
        assert!(matches!(va, VAddr::Global(_)));
    }

    #[test]
    fn stack_marshal() {
        let x = 123;
        let va = VAddr::get(&fake_ctx(), &x as *const _ as usize);
        let mut buf = vec![0u8; va.size()];
        va.marshal(buf.as_mut_ptr());
        let mut vb: VAddr = unsafe { std::mem::zeroed() };
        vb.unmarshal(buf.as_ptr());
        assert_eq!(va, vb);
    }

    #[test]
    fn global_marshal() {
        static X: i32 = 321;
        let va = VAddr::get(&fake_ctx(), &X as *const _ as usize);
        let mut buf = vec![0u8; va.size()];
        va.marshal(buf.as_mut_ptr());
        let mut vb: VAddr = unsafe { std::mem::zeroed() };
        vb.unmarshal(buf.as_ptr());
        assert_eq!(va, vb);
    }
}
