// Implements a structure to hold the current state of the model-checker.
//
//! The state must contain:
//! - The ExecutionGraph;
//! - A recursion stack;
//! - Iterators of where each read/write is trying to do now; The easiest
//! way to do this is to keep only the EventId(tid,n) we are looking at
//! and process it in increasing order of tid and decreasing order of n, and
//! end checking Init. This way the stack is O(N) of memory.
//! - A flag indicating if it is supposed to handle a new event add,
//! or compare it with the trace that is being reconstructed. We call it
//! 'is_reconstructing'.
//!
use crate::helpers::trust_iterator::TrustEventIterator;
use event_utils::{EventId, EventInfo};
use execution_graph::ExecutionGraph;
use lotto::engine::handler::{TaskId, ValuesTypes};
use TrustEventIterator::*;

use serde_derive::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, PartialEq, Clone, Copy)]
pub struct AddressInfo {
    pub init_value: ValuesTypes,
}

impl AddressInfo {
    pub fn new(init_value: ValuesTypes) -> Self {
        Self { init_value }
    }
}

#[derive(Serialize, Deserialize)]
pub struct TrustState {
    /// Recursion stack of TruST.
    /// Contains the event and it's "loop-iteration" pointer.
    pub stack: Vec<TrustStackFrame>,
    pub graph: ExecutionGraph,
    /// Contains a valid topological order for adding the events.
    /// If not empty it implicetly means we are following the trace.
    /// must have: (tid, insertion_order).
    /// `co`-edges are implicitely set because they form a total order
    /// on each address.
    /// `rf`-edges are implicitely set due to the `fr` edges used in the
    /// topological sort.
    /// `po`-edges are trivial.
    /// `fr` edges are implicetely set because we already defined the
    /// `co` and `rf` edges.
    pub rebuild_trace: Vec<(TaskId, EventInfo)>,
    #[serde(skip)]
    pub rebuild_trace_ptr: usize,
}

impl Default for TrustState {
    fn default() -> Self {
        Self::new()
    }
}

impl TrustState {
    pub fn new() -> Self {
        Self {
            stack: Vec::new(),
            graph: ExecutionGraph::new(),
            rebuild_trace: Vec::new(),
            rebuild_trace_ptr: 0,
        }
    }
    #[allow(dead_code)]
    pub fn print_graph(&self) {
        self.graph.print_edges();
    }
    #[allow(dead_code)]
    pub fn print_trace(&self) {
        for (tid, ev_info) in &self.rebuild_trace {
            println!(
                "{:?} {:?}, addr = {:?}",
                tid, ev_info.event_type, ev_info.addr
            );
        }
    }
}

/// The stack holds:
/// - the event being processed (ev_info)
/// - the position of the iterator in the loop:
///     - for a write we need 2 iterators: inner loop (rf) and outer (co-edge).
///       If `read_ptr.is_none()` it means we are not backwards-revisiting.
///     - for a read we only need 1 pointer for the current co-edge (`write_ptr`),
///       while read_ptr is None.
#[derive(Clone, Copy, Serialize, Deserialize, PartialEq, Debug)]
pub struct TrustStackFrame {
    pub tid: usize,
    /// [TODO] We dont need the whole EventInfo here
    pub event_info: EventInfo,
    pub iterator: TrustEventIterator,
}

impl TrustStackFrame {
    pub fn new(tid: usize, event_info: EventInfo, iterator: TrustEventIterator) -> Self {
        Self {
            tid,
            event_info,
            iterator,
        }
    }
    pub fn write_id_from_read_iterator(&self) -> EventId {
        match self.iterator {
            ReadIterator(w) => w,
            _ => panic!("Unexpected iterator type. Must be a ReadIterator"),
        }
    }
}
