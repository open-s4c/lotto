//! # Manage Lotto objects
//!
//! As a convention, all wrappers should wrap directly the internal
//! (non-pointer) type. The reference semantics are implemented using
//! Rust's native references (`&Foo` and `&mut Foo`). Owned objects
//! should be typed [`Owned`]. You should also provide [`Dispose`]
//! instances where appropriate to allow proper RAII.
//!
//! - If a C function returns a non-owning pointer (`foo_t *`), simply
//!   convert it to `&Foo`. Their representations are compatible.
//! - If a C function returns an owning pointer (`foo_t *`), put it
//!   inside `Owned`. (That gives you `Owned<Foo>`.)
//! - Any `&Owned<Foo>` can automatically [`Deref`] to the corresponding
//!   `&Foo`.
//!
//! These conventions are automatically enforced when using [`wrap`]
//! and [`wrap_with_dispose`].
//!
//! ## Examples
//!
//! To wrap a type `foo_t`,
//! ```
//! # struct foo_t;
//! #[repr(transparent)]
//! struct Foo(foo_t);
//! ```
//!
//! Conventions:
//! - `foo_alloc` returns `Owned<Foo>`.
//! - `foo_get_readonly_singleton` returns `&'static Foo`.
//! - `foo_set_value` takes `&mut Foo`.
//! - `foo_get_value` takes `&Foo`.
//! - `foo_clone` takes `&Foo` and returns `Owned<Foo>`. You probably
//!   want to implement `ToOwned<Owned=Owned<T>>` for `Foo`, which in
//!   turn will implement `Clone` for `Owned<T>` automatically.
//!
//! If there's a special deallocator for `foo_t *`, create a `Dispose`
//! instance:
//! ```
//! # use lotto::owned::Dispose;
//! # struct Foo;
//! # struct foo_t;
//! # unsafe fn foo_destroy(ptr: *mut foo_t) {}
//! impl Dispose for Foo {
//!     type Source = foo_t;
//!     unsafe fn dispose(ptr: *mut foo_t) {
//!         unsafe { foo_destroy(ptr); }
//!     }
//! }
//! ```
//! Any [`Owned`] instances will automatically be freed by it.

use core::borrow::{Borrow, BorrowMut};
use core::ops::{Deref, DerefMut};
use std::fmt;

/// Associate Lotto destructors to the wrapper type.
///
/// Intended to be used by [`Owned`].
pub trait Dispose {
    /// The original type.
    ///
    /// For example, the source type of [`crate::Flags`] is
    /// [`crate::raw::flags_t`].
    type Source;
    /// # Safety
    /// `ptr` must be valid.
    unsafe fn dispose(ptr: *mut Self::Source);
}

/// Owned object.
///
/// Similar to [`Box`], it will deallocate the owned object when it
/// goes out of scope, but allows arbitrary deallocator through
/// [`Dispose`].
pub struct Owned<T: Dispose> {
    ptr: *mut T::Source,
}

impl<T: Dispose> Owned<T> {
    /// # Safety
    /// Transfer ownership.
    pub unsafe fn from_raw(ptr: *mut T::Source) -> Owned<T> {
        assert!(!ptr.is_null());
        Owned { ptr }
    }

    /// # Safety
    /// Relinquish ownership.
    pub fn into_raw(self) -> *mut T::Source {
        let ptr = self.ptr;
        std::mem::forget(self);
        ptr
    }
}

impl<T: Dispose> Drop for Owned<T> {
    fn drop(&mut self) {
        unsafe { T::dispose(self.ptr) }
    }
}

impl<T: Dispose> Deref for Owned<T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*(self.ptr as *const T) }
    }
}

impl<T: Dispose> DerefMut for Owned<T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *(self.ptr as *mut T) }
    }
}

impl<T: Dispose> Borrow<T> for Owned<T> {
    fn borrow(&self) -> &T {
        self
    }
}

impl<T: Dispose> BorrowMut<T> for Owned<T> {
    fn borrow_mut(&mut self) -> &mut T {
        self
    }
}

/// Wrap a source Lotto type while enforcing the conventions.
///
/// The wrapper type automatically gets `as_ptr`, `as_mut_ptr`,
/// [`Deref`], and [`DerefMut`].
#[macro_export]
macro_rules! wrap {
    ($wrapper:ident, $source:ty) => {
        #[repr(transparent)]
        #[doc=concat!("Wrapped [`", stringify!($source), "`].")]
        pub struct $wrapper {
            inner: $source,
        }

        impl $wrapper {
            pub fn as_ptr(&self) -> *const $source {
                &self.inner as *const $source
            }

            pub fn as_mut_ptr(&mut self) -> *mut $source {
                &mut self.inner as *mut $source
            }

            /// Convert a raw pointer into a reference to the wrapper.
            pub unsafe fn wrap<'a>(ptr: *const $source) -> &'a $wrapper {
                unsafe { &*(ptr as *const $wrapper) }
            }

            /// Convert a raw pointer into a reference to the wrapper.
            pub unsafe fn wrap_mut<'a>(ptr: *mut $source) -> &'a mut $wrapper {
                unsafe { &mut *(ptr as *mut $wrapper) }
            }
        }

        impl ::core::ops::Deref for $wrapper {
            type Target = $source;
            fn deref(&self) -> &Self::Target {
                &self.inner
            }
        }

        impl ::core::ops::DerefMut for $wrapper {
            fn deref_mut(&mut self) -> &mut Self::Target {
                &mut self.inner
            }
        }

        impl ::core::convert::From<$source> for $wrapper {
            fn from(val: $source) -> $wrapper {
                $wrapper { inner: val }
            }
        }
    };
}

/// Like `wrap!` but with [`Dispose`].
///
/// The [Dispose] implementation is simply a call to the
/// deallocator. If the pointer type does not match `*mut
/// source_type`, you can provide a target pointer type to cast it.
#[macro_export]
macro_rules! wrap_with_dispose {
    ($wrapper:ident, $source:ty, $dealloc:expr) => {
        $crate::wrap_with_dispose!($wrapper, $source, $dealloc, *mut $source);
    };
    ($wrapper:ident, $source:ty, $dealloc:expr, $cast:ty) => {
        $crate::wrap!($wrapper, $source);
        impl $crate::owned::Dispose for $wrapper {
            type Source = $source;
            unsafe fn dispose(ptr: *mut $source) {
                $dealloc(ptr as $cast);
            }
        }
    };
}

impl<T: fmt::Display + Dispose> fmt::Display for Owned<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let r: &T = &*self;
        r.fmt(f)
    }
}

impl<T: fmt::Debug + Dispose> fmt::Debug for Owned<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let r: &T = &*self;
        r.fmt(f)
    }
}

impl<T: Dispose + ToOwned<Owned = Owned<T>>> Clone for Owned<T> {
    fn clone(&self) -> Self {
        let r: &T = &*self;
        r.to_owned()
    }
}
