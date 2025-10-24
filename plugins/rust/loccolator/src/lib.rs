use std::{alloc, cmp, mem, ptr};

extern "C" {
    fn sys_memalign(a: usize, s: usize) -> *mut u8;
    fn sys_realloc(p: *mut u8, s: usize) -> *mut u8;
    fn sys_free(p: *mut u8);
}

pub struct LottoAllocator;

unsafe impl alloc::GlobalAlloc for LottoAllocator {
    unsafe fn alloc(&self, layout: alloc::Layout) -> *mut u8 {
        let size = layout.size();
        let ptr = if size == 0 {
            return layout.align() as *mut u8;
        } else {
            let align = cmp::max(layout.align(), mem::size_of::<*mut u8>());
            sys_memalign(align, size)
        };
        ptr
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: alloc::Layout, new_size: usize) -> *mut u8 {
        if new_size == 0 {
            sys_free(ptr);
            return layout.align() as *mut u8;
        }

        // Try realloc first to avoid memcpy.
        // However, we may have to copy twice when the alignment is lost.
        let ptr = sys_realloc(ptr, new_size);
        if ptr.is_null() {
            return ptr::null_mut();
        }
        if ptr as usize % layout.align() == 0 {
            return ptr;
        }

        // ptr is ok, but alignment is lost. Restore it.
        let aligned_ptr = self.alloc(alloc::Layout::from_size_align_unchecked(
            new_size,
            layout.align(),
        ));
        if aligned_ptr.is_null() {
            self.dealloc(ptr, layout);
            return ptr::null_mut();
        }
        let copy_size = cmp::min(layout.size(), new_size);
        ptr::copy_nonoverlapping(ptr, aligned_ptr, copy_size);
        self.dealloc(ptr, layout);
        aligned_ptr
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: alloc::Layout) {
        if !ptr.is_null() {
            sys_free(ptr);
        }
    }
}
