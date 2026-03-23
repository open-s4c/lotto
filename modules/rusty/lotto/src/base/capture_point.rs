use crate::{base::EventType, wrap};
use lotto_sys as raw;
use std::ffi::CStr;

wrap!(CapturePoint, raw::capture_point);

impl CapturePoint {
    pub fn event_type(&self) -> EventType {
        EventType(self.src_type as u32)
    }

    pub fn function_name(&self) -> &'static str {
        // Safety: from_ptr must only be called on valid C null-terminated strings,
        // and its lifetime must exceed the requested lifetime.
        // This string is assigned from the C side to __func__ which
        // is a valid C string with static lifetime.
        let name = unsafe { CStr::from_ptr(self.func) };
        name.to_str().unwrap()
    }

    /// Obtain the payload without checking.
    pub unsafe fn payload_unchecked<T>(&self) -> Option<&T> {
        (self.__bindgen_anon_1.payload as *const T).as_ref()
    }

    /// Obtain the payload in a type-safe manner.
    ///
    /// This is only possible for a handful of payload types.
    pub fn payload<Payload: PayloadType>(&self) -> Option<&Payload> {
        debug_assert!(Payload::has_correct_event_type(self.event_type()));
        unsafe { self.payload_unchecked::<Payload>() }
    }

    pub fn caller_pc(&self) -> Option<usize> {
        Some(unsafe { self.payload_unchecked::<raw::stacktrace_event>()?.caller as usize })
    }

    pub fn rmw_op(&self) -> Option<u32> {
        match self.event_type().0 {
            raw::EVENT_MA_RMW => unsafe {
                self.payload_unchecked::<raw::ma_rmw_event>()
                    .map(|ev| ev.op as u32)
            },
            _ => None,
        }
    }
}

pub trait PayloadType {
    fn has_correct_event_type(src_type: EventType) -> bool;
}

macro_rules! impl_typed_payload {
    ($payload_type:ty, $pats:pat) => {
        impl PayloadType for $payload_type {
            fn has_correct_event_type(src_type: EventType) -> bool {
                let val = src_type.to_raw_value();
                matches!(val, $pats)
            }
        }
    };
}

impl_typed_payload!(raw::ma_aread_event, raw::EVENT_MA_AREAD);
impl_typed_payload!(raw::ma_awrite_event, raw::EVENT_MA_AWRITE);
impl_typed_payload!(raw::ma_read_event, raw::EVENT_MA_READ);
impl_typed_payload!(raw::ma_write_event, raw::EVENT_MA_WRITE);
impl_typed_payload!(raw::ma_rmw_event, raw::EVENT_MA_RMW);
impl_typed_payload!(raw::ma_xchg_event, raw::EVENT_MA_XCHG);
impl_typed_payload!(
    raw::ma_cmpxchg_event,
    raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK
);
impl_typed_payload!(
    raw::stacktrace_event,
    raw::EVENT_STACKTRACE_ENTER | raw::EVENT_STACKTRACE_EXIT
);
