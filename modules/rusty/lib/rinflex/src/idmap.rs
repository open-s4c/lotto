use std::hash::Hash;

use lotto::collections::FxHashMap;

/// A bidirectional mapping between objects and their ID number.
#[derive(Clone, Debug, Default)]
pub struct IdMap<T> {
    vec: Vec<T>,
    map: FxHashMap<T, usize>,
}

impl<T: Hash + Clone + Eq> IdMap<T> {
    /// Possibly insert a new object and get its id, or directly
    /// return the id of an existing object.
    pub fn put(&mut self, obj: &T) -> usize {
        if let Some(id) = self.map.get(obj) {
            return *id;
        }
        self.vec.push(obj.clone());
        let id = self.vec.len() - 1;
        self.map.insert(obj.clone(), id);
        id
    }

    /// Obtain the stored object by its id.
    pub fn get(&self, id: usize) -> Option<&T> {
        self.vec.get(id)
    }
}
