// Implements common utilities for representing events.
//
use lotto::engine::handler::{AddrSize, Address, ContextInfo, ValuesTypes};
use lotto::log::trace;
use serde_derive::{Deserialize, Serialize};

#[derive(Eq, PartialEq, Hash, Debug, Clone, Copy, PartialOrd, Ord, Serialize, Deserialize)]
/// The event id is given by a tuple:
/// thread-id & number of the event of the thread
/// `n` starts from 0.
pub struct EventId {
    pub tid: usize,
    pub n: usize,
}

impl EventId {
    pub fn new(tid: usize, n: usize) -> Self {
        Self { tid, n }
    }
}

#[derive(PartialEq, Debug, Clone, Copy, Serialize, Deserialize)]
pub enum EventValue {
    /// Value written by a RMW.
    RmwValue(ValuesTypes),
    /// Expected value and value written in case of a success.
    CasValues(ValuesTypes, ValuesTypes),
    /// Value written.
    WriteValue(ValuesTypes),
    /// Other cases, like None events or reads
    NoneValue,
}

impl EventValue {
    pub fn get_rmw_write_value(val: EventValue) -> ValuesTypes {
        match val {
            EventValue::RmwValue(write_val) => write_val,
            EventValue::CasValues(_expected, desired) => desired,
            _ => panic!("Unexpected value type for a read"),
        }
    }
}

#[derive(PartialEq, Debug, Clone, Copy, Serialize, Deserialize)]
pub enum EventType {
    Read,
    Write,
    Error,
    None,
    Init,
    End,
}

#[derive(Clone, Copy, Debug, PartialEq, Serialize, Deserialize)]
pub struct EventInfo {
    /// A id of the order of insertion in the graph
    /// useful for the <_G total order operator.
    pub insertion_order: u64,
    /// address of write/read, if any.
    pub addr: Option<Address>,
    pub addr_length: Option<AddrSize>,
    pub event_type: EventType,
    pub is_atomic: bool,
    pub is_rmw: bool,
    pub val: EventValue,
    // todo: figure out what to do in case failed CAS.
    // do we update the is_rmw of the Read to false? Other ideas?
}

impl EventInfo {
    pub fn init() -> Self {
        Self {
            insertion_order: 0,
            is_atomic: false,
            event_type: EventType::Init,
            addr: None,
            addr_length: None,
            is_rmw: false,
            val: EventValue::NoneValue,
        }
    }
    pub fn none() -> Self {
        Self {
            insertion_order: 0,
            is_atomic: false,
            event_type: EventType::None,
            addr: None,
            addr_length: None,
            is_rmw: false,
            val: EventValue::NoneValue,
        }
    }
    pub fn end() -> Self {
        Self {
            insertion_order: 0,
            is_atomic: false,
            event_type: EventType::End,
            addr: None,
            addr_length: None,
            is_rmw: false,
            val: EventValue::NoneValue,
        }
    }

    pub fn new(
        insertion_order: u64,
        addr: Option<Address>,
        addr_length: Option<AddrSize>,
        event_type: EventType,
        is_atomic: bool,
        is_rmw: bool,
        val: EventValue,
    ) -> Self {
        if event_type == EventType::Read || event_type == EventType::Write {
            assert!(addr.is_some());
        }
        if is_rmw {
            assert!(event_type == EventType::Read || event_type == EventType::Write);
        }
        Self {
            insertion_order,
            addr,
            addr_length,
            event_type,
            is_atomic,
            is_rmw,
            val,
        }
    }
    pub fn is_cas(ev_info: &EventInfo) -> bool {
        matches!(ev_info.val, EventValue::CasValues(_, _))
    }

    pub fn new_from_context(ctx: &ContextInfo) -> Self {
        use lotto::engine::handler::AddressCatEvent::*;
        use lotto::engine::handler::AddressTwoValuesCatEvent::*;
        use lotto::engine::handler::AddressValueCatEvent::*;
        use lotto::engine::handler::EventArgs::*;
        match &ctx.event_args {
            AddressOnly(BeforeRead, addr, size) => {
                EventInfo::new(
                    /* insertion_order */ 0,
                    /* addr */ Some(*addr),
                    /* length */ Some(*size),
                    /* ev_type */ EventType::Read,
                    /* is_atomic */ false,
                    /* is_rmw */ false,
                    EventValue::NoneValue,
                )
            }
            AddressOnly(BeforeARead, addr, size) => EventInfo::new(
                0,
                Some(*addr),
                Some(*size),
                EventType::Read,
                true,
                false,
                EventValue::NoneValue,
            ),
            AddressOnly(BeforeWrite, addr, size) => EventInfo::new(
                0,
                Some(*addr),
                Some(*size),
                EventType::Write,
                false,
                false,
                EventValue::NoneValue,
            ),
            AddressValue(BeforeAWrite, addr, size, value) => EventInfo::new(
                0,
                Some(*addr),
                Some(*size),
                EventType::Write,
                true,
                false,
                EventValue::WriteValue(*value),
            ),
            AddressValue(BeforeRMW, addr, size, value) => {
                // RMW *Never* fails, so we can set `is_rmw = true`
                EventInfo::new(
                    0,
                    Some(*addr),
                    Some(*size),
                    EventType::Read,
                    true,
                    true,
                    EventValue::RmwValue(*value),
                )
            }
            AddressTwoValuesEvent(BeforeCMPXCHG, addr, size, value1, value2) => {
                // In case of a failed RMW, we set `is_rmw = false`, otherwise we set it as true.
                if lotto::engine::handler::get_value_from_addr(addr, size) == *value1 {
                    // Success.
                    trace!(
                        "Success in RMW {:?}, current = {:?}, expected = {:?}",
                        ctx.cat,
                        lotto::engine::handler::get_value_from_addr(addr, size),
                        value1
                    );

                    return EventInfo::new(
                        0,
                        Some(*addr),
                        Some(*size),
                        EventType::Read,
                        true,
                        /* is_rmw */ true,
                        EventValue::CasValues(*value1, *value2),
                    );
                }
                trace!(
                    "Failed RMW, current = {:?}, expected = {:?}",
                    lotto::engine::handler::get_value_from_addr(addr, size),
                    value1
                );
                EventInfo::new(
                    0,
                    Some(*addr),
                    Some(*size),
                    EventType::Read,
                    true,
                    /* is_rmw */ false,
                    EventValue::CasValues(*value1, *value2),
                )
            }
            // TODO: we need to add a way to check if we are at the END event
            // Current idea is look if at task 1, CAT_FUNC_EXIT & 1 active thread.
            _ => EventInfo::none(),
        }
    }
}

#[derive(PartialEq, Clone, Copy, Debug)]
pub enum EdgeType {
    PO,
    CO,
    RF,
    FR,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Edge {
    pub from: EventId,
    pub to: EventId,
    pub edge_type: EdgeType,
}

impl Edge {
    pub fn new(from: EventId, to: EventId, edge_type: EdgeType) -> Self {
        Self {
            from,
            to,
            edge_type,
        }
    }
}

pub fn rmw_will_fail(read_val: EventValue, write_val: EventValue) -> bool {
    match read_val {
        EventValue::CasValues(expected_val, _) => {
            match write_val {
                EventValue::WriteValue(write_value) => expected_val != write_value,
                EventValue::NoneValue => {
                    // This is only true for INIT. In this case, what to do?
                    false
                }
                _ => panic!("Unexpected..."),
            }
        }
        _ => false,
    }
}

pub fn update_rmw_status_of_cas(
    read_ev_info: &mut EventInfo,
    write_val: EventValue,
    init_val: ValuesTypes,
) {
    match read_ev_info.val {
        EventValue::CasValues(expected_val, _) => {
            // update `is_rmw`:
            read_ev_info.is_rmw = match write_val {
                EventValue::WriteValue(write_value) => expected_val == write_value,
                EventValue::NoneValue => {
                    // [TODO] Maybe we should cast to u64 and compare?
                    expected_val == init_val
                }
                _ => panic!("Unexpected..."),
            };
        }
        _ => panic!(
            "Trying to update `rmw` for a event that is not a CAS: {:?}",
            read_ev_info
        ),
    }
}
