//! # Lotto-specific synchronization
//!
//! Lotto guarantees serialized execution and this module provides
//! basic mechanism to take advantage of this.

use std::cell::UnsafeCell;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};
use std::sync::Once;

/// A wrapper for [`Handler`] that is [`Sync`] and globally
/// accessible.
///
/// # Safety
///
/// It is safe only for wrapping handler data as handler access is
/// serialized.
pub struct HandlerWrapper<T> {
    inner: UnsafeCell<DataOrFactory<T>>,
    once: Once,
}

unsafe impl<T: Send> Sync for HandlerWrapper<T> {}

union DataOrFactory<T> {
    data: ManuallyDrop<T>,
    f: ManuallyDrop<fn() -> T>,
}

impl<T> HandlerWrapper<T> {
    pub const fn new(f: fn() -> T) -> Self {
        HandlerWrapper {
            inner: UnsafeCell::new(DataOrFactory {
                f: ManuallyDrop::new(f),
            }),
            once: Once::new(),
        }
    }

    fn init_once(&self) {
        self.once.call_once(|| unsafe {
            let inner = &mut *self.inner.get();
            let f = ManuallyDrop::take(&mut inner.f);
            let data = f();
            inner.data = ManuallyDrop::new(data);
        });
    }

    pub fn get(&self) -> &T {
        self.init_once();
        unsafe { &(*self.inner.get()).data }
    }

    pub unsafe fn get_mut(&self) -> &mut T {
        self.init_once();
        let inner = &mut *self.inner.get();
        &mut inner.data
    }
}

impl<T> Deref for HandlerWrapper<T> {
    type Target = T;

    fn deref(&self) -> &T {
        self.get()
    }
}

impl<T> DerefMut for HandlerWrapper<T> {
    fn deref_mut(&mut self) -> &mut T {
        // Safety: exclusive self.
        unsafe { self.get_mut() }
    }
}

impl<T> Drop for HandlerWrapper<T> {
    fn drop(&mut self) {
        if self.once.is_completed() {
            unsafe {
                ManuallyDrop::drop(&mut (*self.inner.get()).data);
            }
        }
    }
}
