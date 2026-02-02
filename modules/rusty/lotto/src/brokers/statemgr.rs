//! # State management
//!
//! The Rust plugin has a standalone state management system.  To
//! Lotto, all Rust states are stored in one slot, and the Rust plugin
//! will dispatch the states to each Rust component.
use crate::base::record::Record;
use core::ffi::c_void;
use core::mem;
use core::mem::MaybeUninit;
use core::ptr;
use core::ptr::addr_of_mut;
use core::ptr::NonNull;
use core::slice;
use lotto_sys as raw;
use once_cell::sync::Lazy;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;

// Re-export
pub use bincode::{Decode, Encode};
pub use lotto_macros::MarshableNoPrint;

/// Unmarshal a record.
///
/// # Panics
///
/// It panics if called inside a handler.
pub fn record_unmarshal(r: &Record) {
    assert!(!crate::engine::pubsub::during_dispatch());
    unsafe {
        raw::statemgr_record_unmarshal(r.as_ptr());
    }
}

static INITIALIZED: AtomicBool = AtomicBool::new(false);
pub(crate) static PERSISTENT: Lazy<MarshableStateList> =
    Lazy::new(|| MarshableStateList::new(raw::state_type::STATE_TYPE_PERSISTENT));
pub(crate) static CONFIG: Lazy<MarshableStateList> =
    Lazy::new(|| MarshableStateList::new(raw::state_type::STATE_TYPE_CONFIG));
pub(crate) static FINAL: Lazy<MarshableStateList> =
    Lazy::new(|| MarshableStateList::new(raw::state_type::STATE_TYPE_FINAL));

fn assert_initialized() {
    assert!(INITIALIZED.load(Ordering::SeqCst));
}

/// Initialize the Rust state manager.
///
/// This must be called once to register for the slot `RUSTY_ENGINE`.
pub fn init() {
    assert!(!INITIALIZED.load(Ordering::SeqCst));
    PERSISTENT.register();
    CONFIG.register();
    FINAL.register();
    unsafe {
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_AFTER_UNMARSHAL_CONFIG as u16,
            Some(_rust_statemgr_after_unmarshal_config),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_AFTER_UNMARSHAL_FINAL as u16,
            Some(_rust_statemgr_after_unmarshal_final),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_AFTER_UNMARSHAL_PERSISTENT as u16,
            Some(_rust_statemgr_after_unmarshal_persistent),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_BEFORE_MARSHAL_CONFIG as u16,
            Some(_rust_statemgr_before_marshal_config),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_BEFORE_MARSHAL_FINAL as u16,
            Some(_rust_statemgr_before_marshal_final),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
        raw::ps_subscribe(
            lotto_sys::CHAIN_LOTTO as u16,
            lotto_sys::TOPIC_BEFORE_MARSHAL_PERSISTENT as u16,
            Some(_rust_statemgr_before_marshal_persistent),
            lotto_sys::DICE_MODULE_SLOT as i32,
        );
    }
    INITIALIZED.store(true, Ordering::SeqCst);
}

/// Serialize and deserialize data.
///
/// You don't need to implement it manually.  See [`Marshable`] for
/// details.
pub trait Serializable {
    fn unmarshal(&mut self, data: *const u8) -> usize;
    fn marshal(&self, data: *mut u8) -> usize;
    fn size(&self) -> usize;
    fn alloc_size(&self) -> usize;
}

/// Marshable data.
///
/// Logically, [`Serializable`] and [`Marshable`] are one interface
/// (`raw::marshable_t`), but unfortuantely, the current Rust lacks
/// crucial features to enable overrides while providing a default
/// implementation.
///
/// The typical paradigm is:
/// ```
/// # use bincode::{Encode, Decode};
/// # use lotto_macros::MarshableNoPrint;
/// #[derive(Encode, Decode, MarshableNoPrint)]
/// struct Config {
///     enabled: bool
/// }
/// ```
pub trait Marshable: Serializable {
    /// Define how the data is displayed in lotto show.
    fn print(&self) {}

    /// Invoked after the data has been unmarshalled.
    fn after_unmarshal(&mut self) {}
}

#[repr(C)]
pub(crate) struct MarshableStateList {
    m: raw::marshable_t,
    ty: raw::state_type_t,
    pub states: Mutex<Vec<NonNull<dyn Marshable>>>,
}

// Safety: from Rust's perspective, m and ty are read-only (after
// construction).  `states` is protected.
unsafe impl Sync for MarshableStateList {}
unsafe impl Send for MarshableStateList {}

impl MarshableStateList {
    fn new(ty: raw::state_type_t) -> MarshableStateList {
        let mut m = MaybeUninit::<raw::marshable_t>::uninit();
        let ptr = m.as_mut_ptr();
        unsafe {
            addr_of_mut!((*ptr).unmarshal).write(Some(_rust_statemgr_unmarshal));
            addr_of_mut!((*ptr).marshal).write(Some(_rust_statemgr_marshal));
            addr_of_mut!((*ptr).size).write(Some(_rust_statemgr_size));
            addr_of_mut!((*ptr).print).write(Some(_rust_statemgr_print));
            addr_of_mut!((*ptr).alloc_size).write(_rust_statemgr_alloc_size());
            addr_of_mut!((*ptr).next).write(ptr::null_mut());
        }
        let m = unsafe { m.assume_init() };
        MarshableStateList {
            m,
            ty,
            states: Mutex::new(Vec::new()),
        }
    }

    fn register(&self) {
        unsafe {
            raw::statemgr_register(lotto_sys::MODULE_SLOT as i32, &self.m as *const _ as *mut _, self.ty);
        }
    }
}

unsafe extern "C" fn _rust_statemgr_unmarshal(
    m: *mut raw::marshable_t,
    buf: *const c_void,
) -> *const c_void {
    let mut states = (*(m as *const MarshableStateList))
        .states
        .try_lock()
        .expect("single thread");
    let mut next = buf;
    for state in &mut *states {
        let state = state.as_mut();
        unsafe {
            let size = state.unmarshal(next as *const u8);
            next = next.add(size);
        }
    }
    next
}

unsafe extern "C" fn _rust_statemgr_marshal(
    m: *const raw::marshable_t,
    buf: *mut c_void,
) -> *mut c_void {
    assert_initialized();
    let states = (*(m as *const MarshableStateList))
        .states
        .try_lock()
        .expect("single thread");
    let mut next = buf;
    for state in &*states {
        unsafe {
            let state = state.as_ref();
            let size = state.marshal(next as *mut u8);
            next = next.add(size);
        }
    }
    next
}

unsafe extern "C" fn _rust_statemgr_size(m: *const raw::marshable_t) -> usize {
    assert_initialized();
    let states = (*(m as *const MarshableStateList))
        .states
        .try_lock()
        .expect("single thread");
    let mut size = 0usize;
    for state in &*states {
        unsafe {
            let state = state.as_ref();
            size += state.size();
        }
    }
    size
}

unsafe extern "C" fn _rust_statemgr_print(m: *const raw::marshable_t) {
    assert_initialized();
    let states = (*(m as *const MarshableStateList))
        .states
        .try_lock()
        .expect("single thread");
    for state in &*states {
        unsafe {
            let state = state.as_ref();
            state.print();
        }
    }
}

fn _rust_statemgr_alloc_size() -> usize {
    mem::size_of::<MarshableStateList>()
}

impl<T> Serializable for T
where
    T: Decode + Encode + Sized,
{
    fn unmarshal(&mut self, data: *const u8) -> usize {
        let mut rdr = PointerReader { ptr: data };
        let result = bincode::decode_from_reader(&mut rdr, bincode::config::standard())
            .expect(&format!("cannot unmarshal {}", core::any::type_name::<T>()));
        *self = result;
        // Safety: same buffer and the unit is byte.
        unsafe { rdr.ptr.offset_from(data) as usize }
    }

    fn marshal(&self, data: *mut u8) -> usize {
        let len = self.size();
        // Safety: lotto guarantees it's safe to read
        let slice = unsafe { slice::from_raw_parts_mut(data, len) };
        bincode::encode_into_slice(self, slice, bincode::config::standard())
            .expect(&format!("cannot marshal {}", core::any::type_name::<T>()));
        len
    }

    fn size(&self) -> usize {
        let mut counter = bincode::enc::write::SizeWriter::default();
        bincode::encode_into_writer(self, &mut counter, bincode::config::standard()).expect("size");
        counter.bytes_written
    }

    fn alloc_size(&self) -> usize {
        mem::size_of::<raw::marshable_t>() + mem::size_of::<Self>()
    }
}

/// A type-safe interface to the handler's states.
///
/// By providing the pointers, beware: lotto statemgr can modify these
/// data at certain well-defined points.
///
/// Therefore, if these pointers are from a struct, it's advised to
/// use proper interior mutability.
pub trait Stateful {
    type Config: Marshable;
    type Persistent: Marshable;
    type Final: Marshable;

    fn config_ptr(&self) -> *mut Self::Config {
        std::ptr::null_mut()
    }

    fn persistent_ptr(&self) -> *mut Self::Persistent {
        std::ptr::null_mut()
    }

    fn final_ptr(&self) -> *mut Self::Final {
        std::ptr::null_mut()
    }

    unsafe fn config_mut(&self) -> &mut Self::Config {
        let ptr = self.config_ptr();
        if ptr.is_null() {
            panic!("Config pointer is null");
        }
        unsafe { &mut *ptr }
    }

    unsafe fn persistent_mut(&self) -> &mut Self::Persistent {
        let ptr = self.persistent_ptr();
        if ptr.is_null() {
            panic!("Persistent pointer is null");
        }
        unsafe { &mut *ptr }
    }

    unsafe fn final_mut(&self) -> &mut Self::Final {
        let ptr = self.final_ptr();
        if ptr.is_null() {
            panic!("Final pointer is null");
        }
        unsafe { &mut *ptr }
    }
}

struct PointerReader {
    ptr: *const u8,
}

impl bincode::de::read::Reader for PointerReader {
    fn read(&mut self, target: &mut [u8]) -> Result<(), bincode::error::DecodeError> {
        let len = target.len();
        let source = unsafe { slice::from_raw_parts(self.ptr, len) };
        target.copy_from_slice(source);
        unsafe { self.ptr = self.ptr.add(len) }
        Ok(())
    }
}

impl Marshable for () {
    fn print(&self) {}
}

/// Register the states into the state manager, such that they will be
/// managed by Lotto.
pub fn register<Handler: Stateful>(state_owner: &'static Handler)
where
    <Handler as Stateful>::Config: 'static,
    <Handler as Stateful>::Persistent: 'static,
    <Handler as Stateful>::Final: 'static,
{
    {
        let ptr = state_owner.config_ptr();
        if !ptr.is_null() {
            let mut list = CONFIG.states.try_lock().expect("single thread");
            list.push(NonNull::new(ptr).unwrap());
        }
    }
    {
        let ptr = state_owner.final_ptr();
        if !ptr.is_null() {
            let mut list = FINAL.states.try_lock().expect("single thread");
            list.push(NonNull::new(ptr).unwrap());
        }
    }
    {
        let ptr = state_owner.persistent_ptr();
        if !ptr.is_null() {
            let mut list = PERSISTENT.states.try_lock().expect("single thread");
            list.push(NonNull::new(ptr).unwrap());
        }
    }
}

/// A state owner can receive notifications about marshal and
/// unmarshal.
pub trait StateTopicSubscriber {
    fn before_marshal_config(&mut self) {}
    fn before_marshal_persistent(&mut self) {}
    fn before_marshal_final(&mut self) {}
    fn after_unmarshal_config(&mut self) {}
    fn after_unmarshal_persistent(&mut self) {}
    fn after_unmarshal_final(&mut self) {}
}

pub struct StateTopicSubscriberPtr(*mut dyn StateTopicSubscriber);

// SAFETY: This is entirely private. All pointer access is coordinated
// by lotto.
unsafe impl Send for StateTopicSubscriberPtr {}

pub static STATE_TOPIC_SUBSCRIBERS: Mutex<Vec<StateTopicSubscriberPtr>> = Mutex::new(Vec::new());

pub fn subscribe_to_statemgr_topics<StateOwner: StateTopicSubscriber>(
    state_owner: &'static StateOwner,
) {
    let ptr = state_owner as *const dyn StateTopicSubscriber as *mut dyn StateTopicSubscriber;
    let ptr = StateTopicSubscriberPtr(ptr);
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    subscribers.push(ptr);
}

unsafe extern "C" fn _rust_statemgr_after_unmarshal_config(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.after_unmarshal_config();
    }
    raw::ps_err_PS_OK
}

unsafe extern "C" fn _rust_statemgr_after_unmarshal_persistent(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.after_unmarshal_persistent();
    }
    raw::ps_err_PS_OK
}

unsafe extern "C" fn _rust_statemgr_after_unmarshal_final(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.after_unmarshal_final();
    }
    raw::ps_err_PS_OK
}

unsafe extern "C" fn _rust_statemgr_before_marshal_config(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.before_marshal_config();
    }
    raw::ps_err_PS_OK
}

unsafe extern "C" fn _rust_statemgr_before_marshal_persistent(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.before_marshal_persistent();
    }
    raw::ps_err_PS_OK
}

unsafe extern "C" fn _rust_statemgr_before_marshal_final(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    _event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let mut subscribers = STATE_TOPIC_SUBSCRIBERS.try_lock().expect("single thread");
    for ptr in &mut *subscribers {
        let subscriber = unsafe { &mut *ptr.0 };
        subscriber.before_marshal_final();
    }
    raw::ps_err_PS_OK
}
