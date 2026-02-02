use crate::base::TaskId;
use crate::wrap_with_dispose;
use lotto_sys as raw;

wrap_with_dispose!(TidSet, raw::tidset_t, raw::tidset_fini);

impl TidSet {
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
