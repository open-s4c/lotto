use bincode::{Decode, Encode};
use std::cmp::Ordering;

#[derive(Clone, Copy, PartialEq, Eq, Debug, Encode, Decode)]
pub struct U64OrInf(Option<u64>);

impl From<u64> for U64OrInf {
    fn from(value: u64) -> Self {
        U64OrInf(Some(value))
    }
}

impl Default for U64OrInf {
    fn default() -> Self {
        U64OrInf::from(0)
    }
}

impl std::fmt::Display for U64OrInf {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self.0 {
            None => write!(f, "infinity"),
            Some(val) => write!(f, "{}", val),
        }
    }
}

impl PartialOrd for U64OrInf {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        match (self.0, other.0) {
            (Some(a), Some(b)) => a.partial_cmp(&b),
            (Some(_), None) => Some(Ordering::Less),
            (None, Some(_)) => Some(Ordering::Greater),
            (None, None) => None,
        }
    }
}

impl U64OrInf {
    pub fn inf() -> U64OrInf {
        U64OrInf(None)
    }
}
