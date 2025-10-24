
#![allow(clippy::doc_lazy_continuation)]
#![allow(clippy::ptr_arg)]

//! Implements functions that are useful for the implementation of the
//! paper "Truly Stateless, Optimal Dynamic Partial Order Reduction".
//! Access on `https://plv.mpi-sws.org/genmc/popl2022-trust.pdf`,
//! 25 march 2024.
//!
//! Some of its functionalities:
//! - create a new event: O(1)
//! - delete a set of events: O(N)
//! - check for consistency in SC. We check if (po U mo U rf U fr)+ is irreflexive and
//! also if two atomic RWM dont read from the same event. O(N)
//! - check if we can do a foward revisit (add a rf-edge): O(N)
//! - check if we can do a backward revisit: O(N)
//! - get a SC-trace of the program that would generate this same graph: O(N)
//!
//!
//! Most of the functionalities are done by first adding the new event
//! and then checking for consistencie, instead of having a nice
//! data-structure (TODO).
//!
//! TODO: Optimize how we check if we can add a rf-edge,
//! a co-edge or do a backwards revisit. This may be done with the
//! help of a vector-clock/tree-clock.
use core::panic;
use event_utils::EventValue;
use event_utils::{Edge, EdgeType, EventId, EventInfo, EventType};
use lotto::collections::{FxHashMap, FxHashSet};
use lotto::engine::handler::{AddrSize, Address, TaskId, ValuesTypes};
use lotto::log::trace;
use serde_derive::{Deserialize, Serialize};
use traversable_graph::TraversableGraph;
#[cfg(test)]
mod execution_graph_tests;

/// Structure that will hold the execution graph,
/// with its events and the edges between them.
/// We keep only O(N) po and co edges, by keeping only the
/// events that immediatly follow in those orders.
///
#[derive(Serialize, Deserialize)]
pub struct ExecutionGraph {
    /// Holds the event that follows immediatly in
    /// coherence order (aka mo).
    #[serde(skip)]
    co_edge: FxHashMap<EventId, EventId>, // TODO: we may want to move this to `event_info`
    /// Keeps for each address the immediatly after event
    /// in coherence order.
    #[serde(skip)]
    co_edge_init: FxHashMap<Address, EventId>,
    /// Holds the rf mapping: read -> write order.
    #[serde(skip)]
    rf_mapping: FxHashMap<EventId, EventId>, // TODO: we may want to move this to `event_info`
    /// Contains a list with all the co/rf edges.
    #[serde(skip)]
    edges_list: Vec<Edge>, // TODO: This is redundant. We can remove it.
    /// 2D vector indexed by (thread_id, event_number).
    #[serde(skip)]
    event_info: Vec<Vec<EventInfo>>,
    /// Keeps a count of the total number of added events.
    /// This is used in the <_G comparator. Note that when a event is
    /// deleted this doesnt rollback, so it may be greater then the number
    /// of `active` events.
    /// Keep this values serialized.
    cur_id_count: u64,
    /// list of valid ids. Must be updated at insertion and deletion.
    /// TODO: we can delete this, because it is implicit in the
    /// `event_info` size.
    #[serde(skip)]
    valid_ids_list: Vec<EventId>,
    /// hash map with writes to each address.
    #[serde(skip)]
    writes_to_addr: FxHashMap<Address, Vec<EventId>>,
    /// hash map with reads to each address.
    #[serde(skip)]
    reads_to_addr: FxHashMap<Address, Vec<EventId>>,
    /// holds a list of all the RMW reads EventId's.
    /// TODO: properly deal with RMW: We need to handle the case
    /// of a failed RMW.
    #[serde(skip)]
    list_of_rmw_reads: Vec<EventId>,
    /// Initial value at address.
    /// Skip in serialization because some addresses may not appear
    /// any longer. You must initialize this manually.
    #[serde(skip)]
    initial_value: FxHashMap<Address, ValuesTypes>,
}

impl Default for ExecutionGraph {
    fn default() -> Self {
        Self::new()
    }
}

impl ExecutionGraph {
    fn set_rf_edge(&mut self, r: EventId, w: EventId) {
        // sanity checks
        self.assert_is_read(&r);
        self.assert_is_write_or_init(&w);
        assert!(!self.rf_mapping.contains_key(&r));
        assert!(
            (w == EventId::new(0, 0) || self.get_address(&r) == self.get_address(&w))
                && self.get_address(&r).is_some()
        );
        //
        self.rf_mapping.insert(r, w);
        self.add_rf_edge_to_list(r, w);
    }
    fn change_rf_edge(&mut self, r: EventId, w: EventId) {
        let old_w = self.rf_mapping.remove(&r).unwrap();
        self.remove_edge_of_list(old_w, r, EdgeType::RF);
        self.set_rf_edge(r, w);
    }

    fn add_rf_edge_to_list(&mut self, r: EventId, w: EventId) {
        self.edges_list.push(Edge::new(w, r, EdgeType::RF));
    }
    fn add_co_edge_to_list(&mut self, w_from: EventId, w_to: EventId) {
        self.edges_list.push(Edge::new(w_from, w_to, EdgeType::CO));
    }
    fn remove_edge_of_list(&mut self, from: EventId, to: EventId, edge_type: EdgeType) {
        self.edges_list
            .retain(|e| e.from != from || e.to != to || e.edge_type != edge_type);
    }
    fn assert_is_read(&self, ev: &EventId) {
        assert!(self.is_read(ev));
    }
    fn assert_is_write(&self, ev: &EventId) {
        assert!(self.is_write(ev));
    }
    pub fn get_event_info(&self, ev: &EventId) -> EventInfo {
        self.event_info[ev.tid][ev.n]
    }
    fn assert_is_write_or_init(&self, ev: &EventId) {
        let ev_info = self.get_event_info(ev);
        assert!(ev_info.event_type == EventType::Write || ev_info.event_type == EventType::Init);
    }
    fn is_write(&self, ev: &EventId) -> bool {
        let ev_info = self.get_event_info(ev);
        ev_info.event_type == EventType::Write
    }
    fn is_read(&self, ev: &EventId) -> bool {
        let ev_info = self.get_event_info(ev);
        ev_info.event_type == EventType::Read
    }
    fn get_address(&self, ev: &EventId) -> Option<Address> {
        let ev_info = self.get_event_info(ev);
        ev_info.addr
    }
    pub fn get_insertion_order(&self, ev: &EventId) -> u64 {
        self.get_event_info(ev).insertion_order
    }
    pub fn insertion_order_is_unique(&self) -> bool {
        let mut all_ids: FxHashSet<u64> = FxHashSet::default();
        for ev in &self.valid_ids_list {
            all_ids.insert(self.get_event_info(ev).insertion_order);
        }
        all_ids.len() == self.valid_ids_list.len()
    }
    pub fn get_id_count(&self) -> u64 {
        self.cur_id_count
    }
    pub fn set_id_count(&mut self, id_count: u64) {
        self.cur_id_count = id_count;
    }
    pub fn get_number_of_tasks(&self) -> usize {
        assert!(!self.event_info.is_empty());
        self.event_info.len() - 1
    }
    pub fn get_number_of_events_in_task(&self, tid: usize) -> usize {
        self.event_info[tid].len()
    }
    pub fn get_rf_mapping(&self, r: &EventId) -> EventId {
        assert!(self.get_event_info(r).event_type == EventType::Read);
        self.rf_mapping[r]
    }
    /// ## Panic
    /// Panics if no initial value
    ///
    /// TODO: Load the whole cacheline and store it byte-by-byte or store
    /// the whole cacheline and then get the requested value by seing the
    /// address and lenght of the access. This is important to allow mixed
    /// size accesses.
    pub fn get_init_value_of_address(&self, addr: &Address) -> ValuesTypes {
        self.initial_value[addr]
    }
    /// TODO: To allow mixed size accesses we must do loads in the whole cacheline
    pub fn update_init_value_if_not_set(&mut self, addr: Address, val: ValuesTypes) {
        if self.initial_value.contains_key(&addr) {
            return;
        }
        self.initial_value.insert(addr, val);
    }

    pub fn set_co_edge(&mut self, w_prv: EventId, w_new: EventId) {
        // sanity checks
        {
            self.assert_is_write_or_init(&w_prv);
            self.assert_is_write(&w_new);
            assert!(!self.co_edge.contains_key(&w_new));
            assert!(
                (w_prv == EventId::new(0, 0)
                    || self.get_address(&w_prv) == self.get_address(&w_new))
                    && self.get_address(&w_new).is_some()
            );
        }

        if w_prv == EventId::new(0, 0) {
            // INIT:
            let w_new_info = self.get_event_info(&w_new);
            let addr = w_new_info.addr.unwrap();
            if let Some(w_old) = self.co_edge_init.get(&addr).copied() {
                // inserts w_new in the middle of `w_prv -> w_old`.
                self.remove_edge_of_list(w_prv, w_old, EdgeType::CO);
                self.add_co_edge_to_list(w_new, w_old);
                self.co_edge.insert(w_new, w_old);
            }
            self.co_edge_init.insert(addr, w_new);
        } else {
            if let Some(old_w) = self.co_edge.get(&w_prv).copied() {
                self.remove_edge_of_list(w_prv, old_w, EdgeType::CO);
                self.add_co_edge_to_list(w_new, old_w);
                self.co_edge.insert(w_new, old_w);
            }
            self.co_edge.insert(w_prv, w_new);
        }
        self.add_co_edge_to_list(w_prv, w_new);
    }

    /// maybe in the future also pass the id we want as an argument
    /// because of the case we are doing a reconstruction.
    pub fn add_new_event(&mut self, tid: usize, event_info: EventInfo) -> EventId {
        if self.event_info.len() <= tid {
            self.event_info.resize(tid + 1, Vec::new());
        }
        let n = self.event_info[tid].len();
        let ev = EventId::new(tid, n);
        let mut ev_info = event_info;
        ev_info.insertion_order = self.cur_id_count;
        self.event_info.get_mut(tid).unwrap().push(ev_info);
        self.cur_id_count += 1;
        self.valid_ids_list.push(ev);
        ev
    }
    /// adds a new read event that reads-from w_ev
    pub fn add_new_read_event(&mut self, tid: usize, ev_info: EventInfo, w_ev: EventId) -> EventId {
        let mut ev_info = ev_info;
        let addr = ev_info.addr.unwrap();
        if EventInfo::is_cas(&ev_info) {
            // update `rmw` status of CAS.
            let write_val = self.get_event_info(&w_ev).val;
            let init_val = self.get_init_value_of_address(&addr);
            event_utils::update_rmw_status_of_cas(&mut ev_info, write_val, init_val);
        }
        let ev = self.add_new_event(tid, ev_info);
        self.set_rf_edge(ev, w_ev);
        self.reads_to_addr
            .entry(ev_info.addr.unwrap())
            .or_default()
            .push(ev);
        // check if it is `rmw` or not:
        if ev_info.is_rmw {
            assert!(ev_info.is_atomic);
            self.list_of_rmw_reads.push(ev);
        }
        ev
    }

    /// adds a new write event where w_prv is the event preceding in CO.
    pub fn add_new_write_event(
        &mut self,
        tid: usize,
        ev_info: EventInfo,
        w_prv: EventId,
    ) -> EventId {
        let ev = self.add_new_event(tid, ev_info);
        self.set_co_edge(w_prv, ev);
        self.writes_to_addr
            .entry(ev_info.addr.unwrap())
            .or_default()
            .push(ev);
        ev
    }

    /// Currently O(N), assuming O(1) for hash operations.
    /// [TODO] Can we make this O(|deleted|)?
    pub fn remove_set_of_events(&mut self, deleted: &Vec<EventId>) {
        // todo: add some check to see if we are deleting a valid thing.
        let deleted_hash_set = FxHashSet::from_iter(deleted.iter().cloned());
        // update edge_list:
        let mut new_edges_list = Vec::new();
        for edge in &self.edges_list {
            if !deleted_hash_set.contains(&edge.from)
                && !deleted_hash_set.contains(&edge.to)
                && edge.edge_type != EdgeType::CO
            /* co-edges are added later */
            {
                new_edges_list.push(*edge);
            }
        }
        self.edges_list = new_edges_list;

        // update co_edge_list:
        let cur_co_list: FxHashMap<EventId, EventId> = self.co_edge.clone();
        let mut new_co_list = FxHashMap::default();
        let mut new_co_list_init = FxHashMap::default();
        let mut new_events_list: Vec<EventId> =
            cur_co_list.keys().cloned().collect::<Vec<EventId>>();
        new_events_list.retain(|x| !deleted.contains(x));
        new_events_list.push(EventId::new(0, 0)); // INIT;

        // if I keep only O(N) CO edges, we need to be careful when removing...
        // this is still amortized O(N).
        for ev in new_events_list {
            // let ev_info = self.event_info.get(&ev).expect("event exists");
            let ev_info = self.get_event_info(&ev);
            match ev_info.event_type {
                EventType::Write => {
                    let nxt_co_order =
                        Self::get_next_non_deleted_co_order(ev, deleted, &cur_co_list);
                    if let Some(nxt_co_order) = nxt_co_order {
                        new_co_list.insert(ev, nxt_co_order);
                    }
                }
                EventType::Read => {
                    // sanity check
                    assert!(!deleted.contains(&self.rf_mapping[&ev]));
                }
                EventType::Init => {
                    for (addr, nxt_co) in &self.co_edge_init {
                        if !deleted.contains(nxt_co) {
                            new_co_list_init.insert(*addr, *nxt_co);
                        } else {
                            let nxt_co_order =
                                Self::get_next_non_deleted_co_order(*nxt_co, deleted, &cur_co_list);
                            if let Some(nxt_co_order) = nxt_co_order {
                                new_co_list_init.insert(*addr, nxt_co_order);
                            }
                        }
                    }
                }
                _ => { /* nothing */ }
            }
        }

        for (from, to) in &new_co_list {
            self.add_co_edge_to_list(*from, *to);
        }
        for to in new_co_list_init.values() {
            self.add_co_edge_to_list(EventId::new(0, 0) /* INIT */, *to);
        }
        self.co_edge = new_co_list;
        self.co_edge_init = new_co_list_init;

        let mut count_removed_events_in_thread: FxHashMap<usize, usize> = FxHashMap::default();
        let mut min_id_removed_event_in_thread: FxHashMap<usize, usize> = FxHashMap::default();

        for ev in deleted {
            *count_removed_events_in_thread.entry(ev.tid).or_default() += 1;
            let mn = min_id_removed_event_in_thread
                .entry(ev.tid)
                .or_insert(usize::MAX);
            if *mn > ev.n {
                *mn = ev.n;
            }
        }

        let mut changed_addresses: Vec<Address> = deleted
            .iter()
            .filter_map(|ev| self.get_event_info(ev).addr)
            .collect();

        for ev in deleted {
            self.rf_mapping.remove(ev);
        }

        // update event_info:
        for (tid, mn_id) in min_id_removed_event_in_thread.iter() {
            let count = count_removed_events_in_thread[tid];
            assert!(*mn_id + count == self.event_info[*tid].len());
            self.event_info.get_mut(*tid).unwrap().truncate(*mn_id);
        }
        for ev in deleted {
            assert!(self.event_info[ev.tid].len() <= ev.n);
        }
        changed_addresses.sort();
        changed_addresses.dedup();
        for addr in changed_addresses {
            if let Some(reads_to_addr) = self.reads_to_addr.get_mut(&addr) {
                reads_to_addr.retain(|ev| !deleted_hash_set.contains(ev));
            }
            if let Some(writes_to_addr) = self.writes_to_addr.get_mut(&addr) {
                writes_to_addr.retain(|ev| !deleted_hash_set.contains(ev));
            }
        }
        for w in self.rf_mapping.values() {
            assert!(!deleted.contains(w));
        }
        self.list_of_rmw_reads
            .retain(|ev| !deleted_hash_set.contains(ev));
        self.valid_ids_list
            .retain(|ev| !deleted_hash_set.contains(ev));
    }

    pub fn remove_last_event_of_thread(&mut self, tid: usize) {
        self.remove_set_of_events(&vec![self.last_event_of_thread(tid)])
    }
    pub fn last_event_of_thread(&self, tid: usize) -> EventId {
        assert!(!self.event_info[tid].is_empty());
        let pos = self.event_info[tid].len() - 1;
        EventId::new(tid, pos)
    }
    fn get_next_non_deleted_co_order(
        ev: EventId,
        deleted: &Vec<EventId>,
        cur_co_list: &FxHashMap<EventId, EventId>,
    ) -> Option<EventId> {
        if let Some(current_nxt) = cur_co_list.get(&ev) {
            let mut actual_nxt = current_nxt;
            while deleted.contains(actual_nxt) {
                let after_co = cur_co_list.get(actual_nxt);
                match after_co {
                    None => {
                        return None;
                    }
                    Some(after_co) => {
                        actual_nxt = after_co;
                    }
                };
            }
            Some(*actual_nxt)
        } else {
            // still points to nothing
            None
        }
    }

    pub fn new() -> Self {
        let mut cur = Self {
            co_edge: FxHashMap::default(),
            co_edge_init: FxHashMap::default(),
            rf_mapping: FxHashMap::default(),
            edges_list: Vec::new(),
            event_info: Vec::new(),
            cur_id_count: 1,
            valid_ids_list: vec![EventId::new(0, 0)],
            writes_to_addr: FxHashMap::default(),
            reads_to_addr: FxHashMap::default(),
            list_of_rmw_reads: Vec::new(),
            initial_value: FxHashMap::default(),
        };
        cur.event_info.resize(1, Vec::new());
        cur.event_info.get_mut(0).unwrap().push(EventInfo::init());
        cur
    }

    /// As we serialize only a part of the graph, mainly the cur_id_count,
    /// the rest of the variables must be manually set
    pub fn init_after_deserialization(&mut self) {
        self.valid_ids_list = vec![EventId::new(0, 0)];
        self.event_info = vec![vec![EventInfo::init()]]
    }

    fn mocked_events_deleted_on_backwards_revist(&self, r: EventId, w: EventId) -> Vec<EventId> {
        // sanity checks
        self.assert_is_read(&r);
        self.assert_is_write(&w);
        // Deleted <- {e \in G.E | r <G e ∧ ⟨e, a⟩ \not \in G.porf}
        let porf_prefix = self.get_porf_prefix(w);
        let porf_prefix = FxHashSet::from_iter(porf_prefix.iter());
        let deleted = self
            .valid_ids_list
            .iter()
            .cloned()
            .filter(|ev| {
                self.get_event_info(&r).insertion_order < self.get_event_info(ev).insertion_order
                    && !porf_prefix.contains(ev)
            })
            .collect();
        deleted
    }

    pub fn events_deleted_on_backwards_revist(
        &mut self,
        tid: usize,
        ev_info: EventInfo,
        r: EventId,
    ) -> Vec<EventId> {
        let w = self.add_new_event(tid, ev_info);
        let ans = self.mocked_events_deleted_on_backwards_revist(r, w);
        self.remove_last_event(w);
        ans
    }

    /// adds a new write with `w_prv` preceding in `co` and
    /// a rf edge to `r`.
    pub fn add_new_write_event_backwards_revisit(
        &mut self,
        r: EventId,
        w_prv: EventId,
        tid: usize,
        ev_info: EventInfo,
    ) -> EventId {
        let w = self.add_new_write_event(tid, ev_info, w_prv);
        self.change_rf_edge(r, w);
        w
    }
    fn get_po_edges(&self) -> Vec<Edge> {
        let mut edge_list = Vec::new();
        for (tid, list) in self.event_info.iter().enumerate() {
            if tid == 0 {
                continue;
            }
            for n in 0..list.len() {
                if n == 0 {
                    // edge from init:
                    edge_list.push(Edge::new(
                        EventId::new(0, 0),
                        EventId::new(tid, n),
                        EdgeType::PO,
                    ));
                } else {
                    edge_list.push(Edge::new(
                        EventId::new(tid, n - 1),
                        EventId::new(tid, n),
                        EdgeType::PO,
                    ));
                }
            }
        }
        edge_list
    }

    /// porf-prefix including `w`
    fn get_porf_prefix(&self, w: EventId) -> Vec<EventId> {
        let mut edge_list = self.edges_list.clone();
        edge_list.retain(|e| e.edge_type == EdgeType::RF);
        edge_list.extend(self.get_po_edges().iter());
        let g = TraversableGraph::new(&edge_list);
        g.get_events_on_prefix(w)
    }

    /// porf-prefix excluding `w`
    fn get_strict_porf_prefix(&self, w: EventId) -> Vec<EventId> {
        let mut edge_list = self.edges_list.clone();
        edge_list.retain(|e| e.edge_type == EdgeType::RF);
        edge_list.extend(self.get_po_edges().iter());
        let g = TraversableGraph::new(&edge_list);
        g.get_events_on_strict_prefix(w)
    }

    /// Gets the event `co`-after `w`, excluding itself.
    /// If `w` is INIT, this considers all possible Addresses.
    fn get_events_co_after(&self, w: EventId) -> Vec<EventId> {
        self.assert_is_write_or_init(&w);
        if self.get_event_info(&w).event_type == EventType::Init {
            let mut edge_list = self.edges_list.clone();
            edge_list.retain(|e| e.edge_type == EdgeType::CO);
            let g = TraversableGraph::new(&edge_list);
            g.get_events_on_strict_suffix(w)
        } else {
            // write
            let mut v = Vec::new();
            let mut cur = w;
            while let Some(nxt) = self.co_edge.get(&cur) {
                cur = *nxt;
                v.push(cur);
            }
            v
        }
    }

    /// Gets only O(N) `fr`-edges by picking only the first
    /// event in `co`-order that follows $rf^{-1}$. The rest can be found
    /// by taking the transitive closure of (`fr` U `co`).
    ///
    /// ## Complexity
    /// *O(E)*
    fn get_fr_edges(&self) -> Vec<Edge> {
        return self
            .edges_list
            .iter()
            .flat_map(|edge| {
                if edge.edge_type != EdgeType::RF {
                    return vec![].into_iter();
                }
                let next_co = if edge.from == EventId::new(0, 0) {
                    // INIT
                    let ev_info = self.get_event_info(&edge.to);
                    let addr = ev_info.addr.unwrap();
                    self.co_edge_init.get(&addr)
                } else {
                    self.co_edge.get(&edge.from)
                };
                if let Some(next_co) = next_co {
                    if self.get_event_info(next_co).is_rmw && edge.to.tid != next_co.tid {
                        // must also have a edge to the read part
                        // TODO: Rename this to some other thing?
                        // THis is needed because lotto does not perceive the `rmw` as two
                        // events, and adds them both at once. We must add this
                        // extra edge to assert that the trace will not have something like:
                        // {..., R_{RMW}(x), R(x), W_{RMW}(x) ...}
                        let n = next_co.n;
                        assert!(n > 0);
                        let read_part_of_rmw = EventId::new(next_co.tid, n - 1);
                        assert!(
                            self.get_event_info(&read_part_of_rmw).is_rmw
                                && self.get_event_info(&read_part_of_rmw).event_type
                                    == EventType::Read
                        );
                        return vec![
                            Edge::new(edge.to, read_part_of_rmw, EdgeType::FR),
                            Edge::new(edge.to, *next_co, EdgeType::FR),
                        ]
                        .into_iter();
                    }
                    return vec![Edge::new(edge.to, *next_co, EdgeType::FR)].into_iter();
                }
                vec![].into_iter()
            })
            .collect::<Vec<Edge>>();
    }

    fn po_rf_mo_fr_traversable_graph(&self) -> TraversableGraph {
        let mut edge_list = self.edges_list.clone();
        edge_list.retain(|e| e.edge_type == EdgeType::CO || e.edge_type == EdgeType::RF);
        edge_list.extend(self.get_po_edges().iter());
        let fr_edges = self.get_fr_edges();
        edge_list.extend(fr_edges);

        TraversableGraph::new(&edge_list)
    }

    /// Returns a topological sort. Used for testing.
    #[allow(dead_code)]
    fn sc_trace_from_graph(&self) -> Vec<EventId> {
        let g = self.po_rf_mo_fr_traversable_graph();
        match g.get_topological_sort() {
            Ok(v) => v,
            Err(s) => {
                panic!("{s}");
            }
        }
    }
    pub fn trace_from_graph(&self) -> Vec<(TaskId, EventInfo)> {
        let g = self.po_rf_mo_fr_traversable_graph();
        match g.get_topological_sort() {
            Ok(v) => v
                .iter()
                .map(|ev| (TaskId::new(ev.tid as u64), self.get_event_info(ev)))
                .collect(),
            Err(s) => {
                self.print_edges();
                self.print_fr_edges();
                println!("{g}");
                panic!("{s}");
            }
        }
    }

    pub fn is_consistent_sc(&self) -> bool {
        // No two atomic RMW read from the same write.
        // TODO: we can actually keep this as we add the vertices,
        // or check only for the last event.
        if !self.atomicity_of_rmw_is_preserved() {
            return false;
        }
        // the graph is consistent for SC if
        // (po U rf U mo U fr)+ is irreflexive
        let g = self.po_rf_mo_fr_traversable_graph();
        if !g.is_acyclic_and_connected() {
            return false;
        }
        // I think this is it ;)
        // If needed we can add some sanity checks/debug asserts here
        // for example: is the graph connected?
        true
    }

    /// Returns true if there is no Write in between the read and write of
    /// a RMW.
    ///
    /// O(#rmw_reads)
    ///
    /// TODO: to optimize we could actually look only at the LAST added RMW?
    /// Currently not, but maybe with some other changes it works.
    pub fn atomicity_of_rmw_is_preserved(&self) -> bool {
        // In case of failed CAS, we need to set the read as rmw=false.
        if self.list_of_rmw_reads.is_empty() {
            return true;
        }
        for read in &self.list_of_rmw_reads {
            let tid = read.tid;
            let n = read.n;
            assert!(self.get_event_info(read).event_type == EventType::Read);
            assert!(self.get_event_info(read).is_rmw);
            if self.event_info[tid].len() > n + 1 {
                let w_rmw = EventId::new(tid, n + 1);
                assert!(self.get_event_info(&w_rmw).event_type == EventType::Write);
                let w = &self.rf_mapping[read];
                // there should be no event in between the write that the RMW
                // reads from, and the write.
                // so there should be a CO-edge directly from w->w_rmw,
                // where w ->_{rf} r_rmw.
                if *w == EventId::new(0, 0) {
                    let addr = self.get_address(&w_rmw).unwrap();
                    if self.co_edge_init[&addr] != w_rmw {
                        return false;
                    }
                } else if let Some(w_nxt) = self.co_edge.get(w) {
                    if *w_nxt != w_rmw {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        true
    }

    pub fn get_unmatched_rmw(&self) -> Option<(TaskId, Address, AddrSize, EventValue)> {
        let mut ans = None;
        for ev in &self.list_of_rmw_reads {
            let tid = ev.tid;
            let n = ev.n;
            if self.event_info[tid].len() > n + 1 {
                // ok
                assert!(self.event_info[tid][n + 1].is_rmw);
            } else {
                assert!(ans.is_none()); // at most 1 unmatched RMW

                let ev_info = &self.get_event_info(ev);
                let val = EventValue::WriteValue(match ev_info.val {
                    EventValue::RmwValue(write_val) => write_val,
                    EventValue::CasValues(_expected, desired) => desired,
                    _ => panic!("Unexpected value for a RMW"),
                });
                ans = Some((
                    TaskId::new(tid as u64),
                    ev_info.addr.unwrap(),
                    ev_info.addr_length.unwrap(),
                    val,
                ));
            }
        }
        ans
    }

    /// Changes the RMW status of `ev`, if needed.
    pub fn update_is_rmw(&mut self, ev: &EventId, is_rmw: bool) {
        if self.get_event_info(ev).is_rmw == is_rmw {
            return;
        }
        if is_rmw {
            // Update from failed -> rmw
            self.list_of_rmw_reads.push(*ev);
        } else {
            // Use `position` instead of `retain` to assert
            // it is on the list.
            assert!(self.get_event_info(ev).is_rmw);
            let pos = self.list_of_rmw_reads.iter().position(|e| e == ev).unwrap();
            self.list_of_rmw_reads.remove(pos);
        }
        self.event_info[ev.tid][ev.n].is_rmw = is_rmw;
    }

    /// Checks if `ev` was maximally added w.r.t. to `w`.
    /// Definition from paper. The comments included are a
    /// line-by-line reference.
    /// TODO: Optimize this
    fn is_maximally_added(&self, ev: &EventId, w: &EventId) -> bool {
        self.assert_is_write(w);
        // Previous <- {w ′ \in G.E | w' <=G e ∨ ⟨w' , w⟩ \in G.porf}
        let porf = self.get_strict_porf_prefix(*w);
        let mut previous_events = self.valid_ids_list.clone();
        // w' <=_G ev
        previous_events.retain(|w_prime| {
            self.get_event_info(w_prime).insertion_order <= self.get_event_info(ev).insertion_order
        });
        // or w' \in porf
        previous_events.extend(porf.iter());
        // remove duplicates
        previous_events.sort();
        previous_events.dedup();
        // if ∃𝑟 ∈ Previous such that 𝐺.rf(𝑟) = 𝑒 then return false
        for prv_ev in &previous_events {
            let prv_info = self.get_event_info(prv_ev);
            if prv_info.event_type == EventType::Read && &self.rf_mapping[prv_ev] == ev {
                return false;
            }
        }
        // e′ <- if e ∈ 𝐺.R then 𝐺.rf(𝑒) else e
        let e_prime: &EventId = match self.get_event_info(ev).event_type {
            EventType::Read => &self.rf_mapping[ev],
            EventType::Write => ev,
            _ => panic!("Unexpected event type. Expected a read or write"),
        };
        // return e′ ∈ Previous ∧ not exists  w' ∈ Previous. ⟨𝑒 ′ ,𝑤′ ⟩ ∈ 𝐺.co
        if !previous_events.contains(e_prime) {
            return false;
        }
        let events_co_after = if *e_prime == EventId::new(0, 0) {
            let addr = self.get_address(ev).unwrap();
            if let Some(nxt_ev) = self.co_edge_init.get(&addr) {
                let mut v = self.get_events_co_after(*nxt_ev);
                v.push(*nxt_ev);
                v
            } else {
                vec![]
            }
        } else {
            self.get_events_co_after(*e_prime)
        };
        !previous_events
            .iter()
            .any(|prv_ev| events_co_after.contains(prv_ev))
    }

    fn check_for_data_race_sc(&self, _ev: &EventId) -> bool {
        // We can simply remove this and add the `strict-data-race` flag.
        todo!();
    }

    /// Returns true if there is an error, and false otherwise. Checks for:
    /// - data races (TODO)
    pub fn check_for_errors_sc(&self, last_added_ev: &EventId) -> bool {
        assert!(self.is_consistent_sc());
        // Check for data-race:
        let ev_info = self.get_event_info(last_added_ev);
        if ev_info.event_type == EventType::Read || ev_info.event_type == EventType::Write {
            return self.check_for_data_race_sc(last_added_ev);
        }
        false
    }

    /// mocked version of check if we can have a backwards revisit with rf(r) = w.
    /// We mock this because we want to hide the fact that we have to add a
    /// new vertex to check if it is possible.
    fn mocked_can_backwards_revisit(&self, r: EventId, w: EventId) -> bool {
        self.assert_is_read(&r);
        self.assert_is_write(&w);
        assert!(self.get_address(&r) == self.get_address(&w) && self.get_address(&r).is_some());
        assert!(!self.get_porf_prefix(w).contains(&r));
        // sanity: should not have any CO edge yet
        assert!(!self
            .edges_list
            .iter()
            .any(|e| (e.from == w || e.to == w) && e.edge_type == EdgeType::CO));
        // Deleted ← {𝑒 ∈ 𝐺.E | 𝑟 <𝐺 𝑒 ∧ ⟨𝑒, 𝑎⟩ ∉ 𝐺.porf}
        let mut events_to_check = self.mocked_events_deleted_on_backwards_revist(r, w);
        events_to_check.push(r);
        // if \forall e \in Deleted U {r} . IsMaximallyAdded(G, e, a)
        !events_to_check
            .iter()
            .any(|ev| !self.is_maximally_added(ev, &w))
    }

    fn remove_last_event(&mut self, last_ev_id: EventId) {
        // let last_ev_id = self.cur_id_count - 1;
        self.remove_set_of_events(&vec![last_ev_id]);
        self.cur_id_count -= 1;
    }

    /// Returns true if we can make a backwards revisit with rf(r) = w.
    /// assumes r is not in porf prefix of w!!!
    pub fn can_backwards_revisit(&mut self, r: &EventId, tid: usize, ev_info: EventInfo) -> bool {
        let w = self.add_new_event(tid, ev_info);
        let ans = if self.get_porf_prefix(w).contains(r) {
            false
        } else {
            self.mocked_can_backwards_revisit(*r, w)
        };
        self.remove_last_event(w);
        ans
    }

    /// Returns the removed events and removes then from the graph
    /// does not set the RF edge
    /// does not set a CO edge
    pub fn delete_events_of_backwards_revisit(
        &mut self,
        tid: usize,
        ev_info: EventInfo,
        r: EventId,
    ) -> Vec<EventId> {
        let deleted_events = self.events_deleted_on_backwards_revist(tid, ev_info, r);
        self.remove_set_of_events(&deleted_events);
        deleted_events
    }

    /// Performs the backward revisit and returns the Id of the new write.
    /// This will:
    /// - delete some events;
    /// - set the new RF edge;
    /// - set the CO edge from w_prv to this new write;
    /// - return the EventId of the new write;
    pub fn perform_backwards_revist(
        &mut self,
        r: EventId,
        w_prv: EventId,
        tid: usize,
        ev_info: EventInfo,
    ) -> EventId {
        self.delete_events_of_backwards_revisit(tid, ev_info, r);
        self.add_new_write_event_backwards_revisit(r, w_prv, tid, ev_info)
    }

    pub fn can_add_co_order(&mut self, w_prv: EventId, tid: usize, ev_info: EventInfo) -> bool {
        if ev_info.is_rmw {
            assert!(ev_info.event_type == EventType::Write && ev_info.is_atomic);
            let n = self.event_info[tid].len() - 1;
            let r = EventId::new(tid, n);
            let w_rf = self.rf_mapping[&r];
            if w_rf != w_prv {
                // necessary but not enough.
                return false;
            }
        }
        let w = self.add_new_write_event(tid, ev_info, w_prv);
        let ans = self.is_consistent_sc();
        self.remove_last_event(w);
        ans
    }

    /// Checks if we can do the backwards revisit from a new event to `r`,
    /// while having w_prv as the immediate previous in co order.
    /// Assumes that we can make a backwards revisit to 'r' (but maybe to some
    /// other co-edge).
    /// Doesnt remove or add events.
    /// TODO [optimization]: for now we copy the whole graph
    /// and check if performing the backwards revisit it is still consistent.
    /// This is the easiest way, because the revisit will delete some edges
    /// and that creates a lot of corner cases. My previous idea was to
    /// change the rf-edge, and check for consistency. That is wrong because it
    /// may be inconsistent due to fr-edges of deleted events.
    pub fn can_add_co_order_in_backwards_revist(
        &self,
        w_prv: EventId,
        r: EventId,
        tid: usize,
        ev_info: EventInfo,
    ) -> bool {
        self.assert_is_write_or_init(&w_prv);
        self.assert_is_read(&r);
        if ev_info.is_rmw {
            assert!(ev_info.event_type == EventType::Write && ev_info.is_atomic);
            let n = self.event_info[tid].len() - 1;
            let r = EventId::new(tid, n);
            let w_rf = self.rf_mapping[&r];
            if w_rf != w_prv {
                // necessary, but not sufficient.
                return false;
            }
        }
        let mut copy_g = self.copy_graph();
        if copy_g
            .events_deleted_on_backwards_revist(tid, ev_info, r)
            .contains(&w_prv)
        {
            return false;
        }
        copy_g.perform_backwards_revist(r, w_prv, tid, ev_info);
        copy_g.is_consistent_sc()
    }

    /// Returns a copy of the graph.
    /// Use with caution, because this is slow.
    /// Complexity: O(V + E).
    fn copy_graph(&self) -> ExecutionGraph {
        let mut cp = ExecutionGraph::new();
        cp.initial_value = self.initial_value.clone();
        let trace = self.trace_from_graph();
        let mut last_write_to_addr: FxHashMap<Address, EventId> = FxHashMap::default();
        for (tid, ev_info) in trace {
            let tid = u64::from(tid) as usize;
            if tid == 0 {
                continue;
            }
            let lastest_write = match last_write_to_addr.get(&ev_info.addr.unwrap()) {
                Some(w) => *w,
                None => EventId::new(0, 0),
            };
            let ev: EventId = match ev_info.event_type {
                EventType::Read => cp.add_new_read_event(tid, ev_info, lastest_write),
                EventType::Write => {
                    let w = cp.add_new_write_event(tid, ev_info, lastest_write);
                    last_write_to_addr.insert(ev_info.addr.unwrap(), w);
                    w
                }
                _ => {
                    panic!(
                        "Unexpected type in copy_graph {:?}. Expected READ or WRITE.",
                        ev_info.event_type
                    );
                }
            };
            cp.update_insertion_order(ev, self.get_insertion_order(&ev));
        }
        cp
    }

    pub fn can_add_rf_edge(&mut self, w_rf: EventId, tid: usize, ev_info: EventInfo) -> bool {
        let mut ev_info = ev_info;
        let addr = ev_info.addr.unwrap();
        if EventInfo::is_cas(&ev_info) {
            // check if it actually will be `rmw` if this edge is added:
            let init_val = self.get_init_value_of_address(&addr);
            let write_val = self.get_event_info(&w_rf).val;
            event_utils::update_rmw_status_of_cas(&mut ev_info, write_val, init_val);
        }
        if ev_info.is_rmw {
            // We must also ensure that w_rf is not already read by another rmw.
            // If so, it must be possible to re-direct the read from it to our write
            // as well.
            // For more details read the bottom of page 19 at
            // https://plv.mpi-sws.org/genmc/popl2022-trust.pdf
            // So we check if:
            // (1) The following event in co-order is a RMW write;
            // (2) We can change the `rf` edge from its read to the
            // write of this new RMW.
            let nxt_w = if self.get_event_info(&w_rf).event_type == EventType::Init {
                self.co_edge_init.get(&ev_info.addr.unwrap())
            } else {
                self.co_edge.get(&w_rf)
            };

            if let Some(nxt_w) = nxt_w {
                if self.get_event_info(nxt_w).is_rmw {
                    assert!(nxt_w.n > 0);
                    let nxt_r = EventId::new(nxt_w.tid, nxt_w.n - 1);
                    if !self.can_exchange_rmw_edges(
                        nxt_r,
                        w_rf,
                        tid,
                        ev_info.addr.unwrap(),
                        ev_info.addr_length.unwrap(),
                    ) {
                        return false;
                    }
                }
            }
        }
        let r = self.add_new_read_event(tid, ev_info, w_rf);
        let ans = self.is_consistent_sc();
        self.remove_last_event(r);
        ans
    }

    /// Returns true if we can make the new RMW from `tid` to `addr` to
    /// read from `rmw_w`, and `old_read` to read-from the new RMW.
    fn can_exchange_rmw_edges(
        &mut self,
        old_read: EventId,
        rmw_w: EventId,
        tid: usize,
        addr: Address,
        length: AddrSize,
    ) -> bool {
        self.assert_is_read(&old_read);
        self.assert_is_write_or_init(&rmw_w);
        let mut g = self.copy_graph();
        g.add_new_read_event(tid, ExecutionGraph::mocked_rmw_read(addr, length), rmw_w);
        g.can_backwards_revisit(
            &old_read,
            tid,
            ExecutionGraph::mocked_rmw_write(addr, length),
        ) && g.can_add_co_order_in_backwards_revist(
            rmw_w,
            old_read,
            tid,
            ExecutionGraph::mocked_rmw_write(addr, length),
        )
    }

    /// Returns a copy of the events that are reading from this
    /// address.
    pub fn events_reading_from_addr(&self, ev_info: EventInfo) -> Vec<EventId> {
        assert!(ev_info.event_type == EventType::Write); // for now we only expect this from a write.
        let addr = ev_info.addr.unwrap();
        if let Some(reads_to_addr) = self.reads_to_addr.get(&addr) {
            return reads_to_addr.to_vec();
        }
        Vec::new()
    }

    /// Returns a copy of the events that are writing to this
    /// address, **including** the INIT.
    pub fn events_writing_to_addr_and_init(&self, ev_info: EventInfo) -> Vec<EventId> {
        let addr = ev_info.addr.unwrap();
        if let Some(writes_to_addr) = self.writes_to_addr.get(&addr) {
            let mut v = writes_to_addr.to_vec();
            v.push(EventId::new(0, 0));
            return v;
        }
        vec![EventId::new(0, 0)]
    }

    pub fn is_maximal_write(&self, ev: &EventId) -> bool {
        // is maximal write iff there is no co-after write.
        !self.co_edge.contains_key(ev)
    }

    /// Returns the number of events in the graph, including the INIT
    /// event.
    pub fn num_events_including_init(&self) -> usize {
        self.valid_ids_list.len()
    }

    /// Changes the `insertion_order` of `ev`. It also updates
    /// `cur_id_count` to make sure there will be no duplicate ids.
    pub fn update_insertion_order(&mut self, ev: EventId, insertion_order: u64) {
        self.event_info
            .get_mut(ev.tid)
            .unwrap()
            .get_mut(ev.n)
            .unwrap()
            .insertion_order = insertion_order;
        if self.cur_id_count <= insertion_order {
            self.cur_id_count = insertion_order + 1;
        }
        if !(ev.n == 0 || insertion_order > self.event_info[ev.tid][ev.n - 1].insertion_order) {
            self.print_edges();
            self.print_trace();
        }
        // sanity test
        assert!(ev.n == 0 || insertion_order > self.event_info[ev.tid][ev.n - 1].insertion_order)
    }

    pub fn graphs_are_equivalent(graph_1: &ExecutionGraph, graph_2: &ExecutionGraph) -> bool {
        let tmax = graph_1.event_info.len();
        assert!(tmax == graph_2.event_info.len());
        for tid in 1..tmax {
            let sz = graph_1.event_info[tid].len();
            assert!(sz == graph_2.event_info[tid].len());
            for n in 0..sz {
                let ev_info_1 = graph_1.event_info[tid][n];
                let ev_info_2 = graph_2.event_info[tid][n];
                assert!(ev_info_1.event_type == ev_info_2.event_type);
                assert!(ev_info_1.is_atomic == ev_info_2.is_atomic);
                assert!(ev_info_1.is_rmw == ev_info_2.is_rmw);
            }
        }
        let mut edges_1 = graph_1.edges_list.clone();
        let mut edges_2 = graph_2.edges_list.clone();
        edges_1.sort_by_key(|k| (k.from, k.to));
        edges_1.dedup();
        edges_2.sort_by_key(|k| (k.from, k.to));
        edges_2.dedup();
        assert!(edges_1 == edges_2);
        true
    }

    fn mocked_rmw_read(addr: Address, length: AddrSize) -> EventInfo {
        EventInfo::new(
            0,
            Some(addr),
            Some(length),
            EventType::Read,
            /* is_atomic */ true,
            /* is_rmw */ true,
            EventValue::NoneValue, // mocked, so this should not be used...
        )
    }
    fn mocked_rmw_write(addr: Address, length: AddrSize) -> EventInfo {
        EventInfo::new(
            0,
            Some(addr),
            Some(length),
            EventType::Write,
            /* is_atomic */ true,
            /* is_rmw */ true,
            EventValue::NoneValue,
        )
    }

    //    FOR DEBUG

    pub fn is_empty(&self) -> bool {
        self.valid_ids_list.is_empty()
    }

    pub fn print_edges(&self) {
        trace!("Ids = {:?}", self.valid_ids_list);
        trace!("Edges of atomic events: ");
        for edge in &self.edges_list {
            if self.get_event_info(&edge.from).is_atomic != self.get_event_info(&edge.to).is_atomic
            {
                trace!("{:?} to {:?} {:?}", edge.from, edge.to, edge.edge_type);
                // assert!(edge.from == EventId::new(0, 0));
            }
            if self.get_event_info(&edge.from).is_atomic {
                trace!("{:?} to {:?} {:?}", edge.from, edge.to, edge.edge_type);
            }
        }
    }
    pub fn print_trace(&self) {
        let trace = self.trace_from_graph();
        trace!("Trace of graph:");
        for (tid, ev_info) in &trace {
            if !ev_info.is_atomic {
                // continue;
            }
            trace!(
                "{:?} {:?}{:?}, insertion_order = {:?}, is_atomic = {:?}, addr = {:?}",
                tid,
                ev_info.event_type,
                if ev_info.is_rmw {
                    "_RMW".to_string()
                } else {
                    "".to_string()
                },
                ev_info.insertion_order,
                ev_info.is_atomic,
                ev_info.addr
            );
        }
    }
    pub fn get_valid_ids(&self) -> Vec<EventId> {
        self.valid_ids_list.clone()
    }
    pub fn has_edge(&self, from: EventId, to: EventId, edge_type: EdgeType) -> bool {
        self.edges_list.contains(&Edge::new(from, to, edge_type))
    }
    pub fn print_fr_edges(&self) {
        let edges = self.get_fr_edges();
        trace!("FR edges:");
        for ed in &edges {
            trace!("{:?}, {:?} - FR", ed.from, ed.to);
        }
    }
}
