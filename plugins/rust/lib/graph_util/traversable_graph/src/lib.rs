// Implements functions on a traversable graph.

use event_utils::Edge;
use event_utils::EventId;
use rustc_hash::FxHashMap;

/// Library with Graphs functionalities, such as
/// topological_sorting and getting reachable vertices.
#[derive(Default)]
pub struct TraversableGraph {
    /// adjency list
    adj: FxHashMap<EventId, Vec<EventId>>,
    /// reverse of the adjcency list. For every edge <a,b>
    /// there is a reverse edge <b,a>.
    rev_adj: FxHashMap<EventId, Vec<EventId>>,
    /// list of all the ids represented in this graph
    valid_ids: Vec<EventId>,
}

impl TraversableGraph {
    #[allow(dead_code)]
    /// Use with caution. You must assert that ev1 and ev2 are already
    /// in `valid_ids` list. Used in the tests.
    fn add_edge(&mut self, ev1: EventId, ev2: EventId) {
        self.adj.entry(ev1).or_default().push(ev2);
        self.rev_adj.entry(ev2).or_default().push(ev1);
    }

    pub fn new(edge_list: &Vec<Edge>) -> Self {
        let mut valid_ids = Vec::new();
        let mut adj: FxHashMap<EventId, Vec<EventId>> = FxHashMap::default();
        let mut rev_adj: FxHashMap<EventId, Vec<EventId>> = FxHashMap::default();
        // println!("Graph: ");
        for edge in edge_list {
            valid_ids.push(edge.from);
            valid_ids.push(edge.to);
            adj.entry(edge.from).or_default().push(edge.to);
            rev_adj.entry(edge.to).or_default().push(edge.from);
            // println!("{:?} -> {:?} {:?}", edge.from, edge.to, edge.edge_type);
        }

        valid_ids.sort();
        valid_ids.dedup();

        Self {
            valid_ids,
            adj,
            rev_adj,
        }
    }

    /// Returns any possible topological sort.
    /// Could also add some sorting to enforce this is the
    /// lexicographically smallest/largest if needed.
    ///
    /// Returns an Error in case the graph has cycles or is not connected.
    /// In particular, self-loops are NOT allowed.
    pub fn get_topological_sort(&self) -> Result<Vec<EventId>, String> {
        let mut deg: FxHashMap<EventId, u64> = FxHashMap::default();
        let mut toposort_list = Vec::with_capacity(self.adj.len());

        for id in &self.valid_ids {
            if let Some(adj_list) = self.adj.get(id) {
                deg.insert(*id, adj_list.len() as u64);
            } else {
                toposort_list.push(*id);
            }
        }

        // todo: make this prettier
        let mut ptr = 0;
        while ptr < toposort_list.len() {
            let cur_ev = toposort_list[ptr];
            if let Some(neighbor_list) = self.rev_adj.get(&cur_ev) {
                for neighbor in neighbor_list {
                    let cur_deg = deg[neighbor];
                    if cur_deg > 1 {
                        deg.insert(*neighbor, cur_deg - 1);
                    } else {
                        toposort_list.push(*neighbor);
                    }
                }
            }
            ptr += 1;
        }

        if toposort_list.len() != self.valid_ids.len() {
            // cycle was found
            return Err(
                "Invalid topological sort. Provided graph is not acyclic or is not connected!"
                    .to_string(),
            );
        }
        toposort_list.reverse();
        // TODO: For now we add a sanity check. This can be removed later.
        // assert!(self.check_valid_toposort(&toposort_list));
        Ok(toposort_list)
    }

    pub fn is_acyclic_and_connected(&self) -> bool {
        // the graph is acyclic iff it has a topological order
        self.get_topological_sort().is_ok()
    }

    fn get_reachable_events(
        &self,
        adj_list: &FxHashMap<EventId, Vec<EventId>>,
        source: EventId,
    ) -> Vec<EventId> {
        let mut vis: FxHashMap<EventId, bool> = FxHashMap::default();
        // Bread-first-search:
        let mut list_events: Vec<EventId> = vec![source];
        let mut ptr = 0;
        vis.insert(source, true);
        while ptr < list_events.len() {
            let cur_ev = &list_events[ptr];
            if let Some(adj_list) = adj_list.get(cur_ev) {
                for neighbor in adj_list {
                    vis.entry(*neighbor).or_insert_with(|| {
                        list_events.push(*neighbor);
                        true
                    });
                }
            }
            ptr += 1;
        }
        // end of BFS
        list_events.retain(|ev| *ev != source);
        list_events
    }
    /// Returns the list of events that imply `source`,
    /// that is, there is a path from v -> source.
    ///
    /// **DOESNT** include source itself!
    pub fn get_events_on_strict_prefix(&self, source: EventId) -> Vec<EventId> {
        self.get_reachable_events(&self.rev_adj, source)
    }

    /// Returns the list of events that imply `source`,
    /// that is, there is a path from v -> source.
    /// **INCLUDES** source.
    pub fn get_events_on_prefix(&self, source: EventId) -> Vec<EventId> {
        let mut pref = self.get_events_on_strict_prefix(source);
        pref.push(source);
        pref
    }

    pub fn get_events_on_strict_suffix(&self, source: EventId) -> Vec<EventId> {
        self.get_reachable_events(&self.adj, source)
    }

    pub fn check_valid_toposort(&self, list: &Vec<EventId>) -> bool {
        if list.len() != self.valid_ids.len() {
            return false;
        }
        let mut vis = FxHashMap::default();
        for id in &self.valid_ids {
            vis.insert(*id, false);
        }
        for ev in list {
            let old_val = vis.insert(*ev, true);
            assert!(!old_val.unwrap());
            if let Some(rev_adj_list) = self.rev_adj.get(ev) {
                for neighbor in rev_adj_list {
                    if !vis[neighbor] {
                        return false;
                    }
                }
            }
        }
        true
    }

    // DEBUG
    pub fn print_graph(&self) {
        for id in &self.valid_ids {
            if let Some(to) = self.adj.get(id) {
                println!("id = {:?} - list = {:?}", id, to);
            }
        }
    }
}
use std::fmt;
use string_builder::Builder;

impl fmt::Display for TraversableGraph {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let mut builder = Builder::default();
        for id in &self.valid_ids {
            if let Some(to) = self.adj.get(id) {
                builder.append(format!("id = {:?}, list = {:?}\n", id, to));
            }
        }
        write!(f, "{}", builder.string().unwrap())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_topological_sort() {
        let mut g = TraversableGraph::default();
        g.add_edge(EventId::new(0, 0), EventId::new(0, 1));
        g.add_edge(EventId::new(0, 0), EventId::new(0, 2));
        g.add_edge(EventId::new(0, 1), EventId::new(0, 2));
        g.add_edge(EventId::new(0, 2), EventId::new(0, 3));
        g.add_edge(EventId::new(0, 3), EventId::new(0, 4));
        g.add_edge(EventId::new(0, 4), EventId::new(0, 5));
        g.add_edge(EventId::new(0, 0), EventId::new(0, 5));
        /*
               /-----------------5
          0 ---1----\         4-/
               \-----2----3--/
        */
        g.valid_ids = vec![
            EventId::new(0, 0),
            EventId::new(0, 1),
            EventId::new(0, 2),
            EventId::new(0, 3),
            EventId::new(0, 4),
            EventId::new(0, 5),
        ];
        assert!(g.check_valid_toposort(&g.get_topological_sort().unwrap()));
    }

    /// Tests a lot of random graphs. It asserts there is no
    /// cycle by adding edges from `a` to `b` if `a < b`
    #[test]
    fn stress_topological_sort() {
        use rand::rngs::StdRng;
        use rand::Rng;
        use rand::SeedableRng;

        for test_case in 0..1000 {
            let mut random = StdRng::seed_from_u64(test_case);
            let mut g = TraversableGraph::default();
            let mut ids: Vec<EventId> = Vec::new();
            for _ in 0..50 {
                let num = random.gen_range(0..=100);
                ids.push(EventId::new(0, num));
            }
            ids.sort();
            ids.dedup();
            let len = ids.len();
            for _ in 0..50 {
                let ev1 = ids[random.gen_range(0..len)];
                let ev2 = ids[random.gen_range(0..len)];
                if ev1 < ev2 {
                    g.add_edge(ev1, ev2);
                }
                // else if ev2 > ev1 {
                //     g.add_edge(ev2, ev1);
                // }
            }
            g.valid_ids = ids;
            assert!(g.check_valid_toposort(&g.get_topological_sort().unwrap()));
        }
    }

    #[test]
    fn test_events_on_prefix() {
        let mut g = TraversableGraph::default();
        g.add_edge(EventId::new(0, 0), EventId::new(0, 1));
        g.add_edge(EventId::new(0, 0), EventId::new(0, 2));
        g.add_edge(EventId::new(0, 1), EventId::new(0, 2));
        g.add_edge(EventId::new(0, 2), EventId::new(0, 3));
        g.add_edge(EventId::new(0, 3), EventId::new(0, 4));
        g.add_edge(EventId::new(0, 4), EventId::new(0, 5));
        g.add_edge(EventId::new(0, 0), EventId::new(0, 5));
        g.valid_ids = vec![
            EventId::new(0, 0),
            EventId::new(0, 1),
            EventId::new(0, 2),
            EventId::new(0, 3),
            EventId::new(0, 4),
            EventId::new(0, 5),
        ];
        /*
               /-----------------5
          0 ---1----\         4-/
               \-----2----3--/
        */
        assert!(g.get_events_on_strict_prefix(EventId::new(0, 0)) == Vec::new());
        let mut v = g.get_events_on_strict_prefix(EventId::new(0, 1));
        v.sort();
        assert!(v == vec![EventId::new(0, 0)]);
        let mut v = g.get_events_on_strict_prefix(EventId::new(0, 2));
        v.sort();
        assert!(v == vec![EventId::new(0, 0), EventId::new(0, 1)]);
        let mut v = g.get_events_on_strict_prefix(EventId::new(0, 3));
        v.sort();
        assert!(v == vec![EventId::new(0, 0), EventId::new(0, 1), EventId::new(0, 2)]);
        let mut v = g.get_events_on_strict_prefix(EventId::new(0, 4));
        v.sort();
        assert!(
            v == vec![
                EventId::new(0, 0),
                EventId::new(0, 1),
                EventId::new(0, 2),
                EventId::new(0, 3)
            ]
        );
        let mut v = g.get_events_on_strict_prefix(EventId::new(0, 5));
        v.sort();
        assert!(
            v == vec![
                EventId::new(0, 0),
                EventId::new(0, 1),
                EventId::new(0, 2),
                EventId::new(0, 3),
                EventId::new(0, 4)
            ]
        );
    }
}
