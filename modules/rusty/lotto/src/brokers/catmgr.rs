//! # Category Management
use crate::base::category::Category;
use core::ptr;
use lotto_sys as raw;
use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::sync::Once;

pub struct CategoryKey {
    name: &'static CStr,
    cat: MaybeUninit<Category>,
    once: Once,
}

impl CategoryKey {
    pub const fn new(name: &'static CStr) -> CategoryKey {
        CategoryKey {
            name,
            cat: MaybeUninit::uninit(),
            once: Once::new(),
        }
    }

    pub fn get(&self) -> Category {
        self.once.call_once(|| unsafe {
            let cat = raw::new_category(self.name.as_ptr());
            let cat_ptr = self.cat.as_ptr() as *mut _;
            ptr::write(cat_ptr, cat);
        });
        unsafe { self.cat.assume_init() }
    }
}
