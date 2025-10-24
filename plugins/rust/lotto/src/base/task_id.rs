use bincode::{Decode, Encode};
use serde_derive::{Deserialize, Serialize};

#[derive(
    Debug, PartialEq, PartialOrd, Clone, Copy, Eq, Hash, Ord, Serialize, Deserialize, Encode, Decode,
)]
pub struct TaskId(pub u64);

use std::fmt::Display;
use std::fmt::Formatter;

impl Display for TaskId {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), std::fmt::Error> {
        write!(f, "{}", self.0)
    }
}

impl TaskId {
    pub fn new(tid: u64) -> Self {
        Self(tid)
    }
}

impl From<TaskId> for u64 {
    fn from(tid: TaskId) -> Self {
        tid.0
    }
}

impl From<TaskId> for usize {
    fn from(tid: TaskId) -> Self {
        tid.0 as usize
    }
}
