//! # Category Management
use crate::base::category::Category;
use core::ptr;
use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Once;

static NEXT_CATEGORY: AtomicU32 = AtomicU32::new(Category::CAT_END_.0);

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
            let cat = Category::from_raw(NEXT_CATEGORY.fetch_add(1, Ordering::Relaxed));
            let cat_ptr = self.cat.as_ptr() as *mut _;
            ptr::write(cat_ptr, cat);
        });
        unsafe { self.cat.assume_init() }
    }
}
