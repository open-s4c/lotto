use crate::base::TaskId;
use crate::wrap;
use core::mem::MaybeUninit;
use lotto_sys as raw;

// [`TidSet`] shouldn't have owning references.

wrap!(TidSet, raw::tidset_t);

impl Drop for TidSet {
    fn drop(&mut self) {
        unsafe {
            raw::tidset_fini(self as *mut TidSet as *mut raw::tidset_t);
        }
    }
}

impl std::fmt::Display for TidSet {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let v: Vec<_> = self.iter().map(|id| id.0).collect();
        write!(f, "{:?}", v)
    }
}

impl Clone for TidSet {
    fn clone(&self) -> Self {
        let mut result = TidSet::new();
        result.copy(self);
        result
    }
}

impl TidSet {
    pub fn new() -> TidSet {
        let mut obj: MaybeUninit<TidSet> = MaybeUninit::uninit();
        unsafe {
            raw::tidset_init(obj.as_mut_ptr() as *mut _);
            obj.assume_init()
        }
    }

    pub fn size(&self) -> usize {
        unsafe { raw::tidset_size(self.as_ptr()) }
    }

    pub fn get(&self, idx: usize) -> Option<TaskId> {
        let res = unsafe { raw::tidset_get(self.as_ptr(), idx) };
        if res == 0 {
            None
        } else {
            Some(TaskId(res))
        }
    }

    pub fn iter(&self) -> Iter<'_> {
        Iter { tset: self, idx: 0 }
    }

    pub fn has(&self, tid: TaskId) -> bool {
        unsafe { raw::tidset_has(self.as_ptr(), tid.0) }
    }

    pub fn remove(&mut self, tid: TaskId) -> bool {
        unsafe { raw::tidset_remove(self.as_mut_ptr(), tid.0) }
    }

    pub fn clear(&mut self) {
        unsafe { raw::tidset_clear(self.as_mut_ptr()) }
    }

    pub fn set(&mut self, idx: usize, tid: TaskId) {
        unsafe { raw::tidset_set(self.as_mut_ptr(), idx, tid.0) }
    }

    pub fn copy(&mut self, other: &TidSet) {
        unsafe { raw::tidset_copy(self.as_mut_ptr(), other.as_ptr()) }
    }
}

impl IntoIterator for TidSet {
    type Item = TaskId;
    type IntoIter = IntoIter;
    fn into_iter(self) -> Self::IntoIter {
        IntoIter { tset: self, idx: 0 }
    }
}

pub struct IntoIter {
    tset: TidSet,
    idx: usize,
}

impl Iterator for IntoIter {
    type Item = TaskId;

    fn next(&mut self) -> Option<Self::Item> {
        if self.idx >= self.tset.size() {
            None
        } else {
            let result = self.tset.get(self.idx);
            self.idx += 1;
            result
        }
    }
}

pub struct Iter<'a> {
    tset: &'a TidSet,
    idx: usize,
}

impl<'a> Iterator for Iter<'a> {
    type Item = TaskId;
    fn next(&mut self) -> Option<Self::Item> {
        if self.idx >= self.tset.size() {
            None
        } else {
            let result = self.tset.get(self.idx);
            self.idx += 1;
            result
        }
    }
}
