use bincode::{Decode, Encode};
use lotto_sys as raw;
use std::ffi::CStr;
use std::hash::Hash;

/// Event type
#[derive(Encode, Decode, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct EventType(pub(crate) u32);

impl EventType {
    pub fn from_raw_value(val: u32) -> Self {
        EventType(val)
    }

    pub fn to_raw_value(self) -> u32 {
        self.0
    }

    pub fn name(&self) -> String {
        let ty: raw::type_id = self
            .0
            .try_into()
            .expect("semantic event id must fit into type_id");
        let mut name = unsafe { CStr::from_ptr(raw::ps_type_str(ty)) }
            .to_string_lossy()
            .into_owned();
        if let Some(stripped) = name.strip_prefix("EVENT_") {
            name = stripped.to_owned();
        }
        name
    }
}

impl std::fmt::Display for EventType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.name())?;
        Ok(())
    }
}

impl std::fmt::Debug for EventType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}/{}", self.name(), self.0)?;
        Ok(())
    }
}
