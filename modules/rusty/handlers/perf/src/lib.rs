//! A Lotto Handler that will record the number of times each category is
//! is called, the number of calls for each PC, the number of chpt of
//! each task and other perf parameters.
//!
//!

use lotto::collections::FxHashMap;
use lotto::engine::handler::ExecuteHandler;
use lotto::engine::handler::{ContextInfo, TaskId};
use lotto::log::{debug, trace, warn};
use lotto::raw::{base_category, category_t};
use lotto::raw::{map_address_t, memory_map_address_lookup};
use std::sync::LazyLock;

static HANDLER: LazyLock<PerfHandler> = LazyLock::new(|| PerfHandler::default());

#[derive(Default)]
struct PerfHandler {
    /// Number of captures per category.
    count_category: FxHashMap<category_t, u64>,
    /// Number of capture chpt for each task.
    count_events_for_task: FxHashMap<TaskId, u64>,
    /// Number of times each pc creates a captured chpt.
    count_program_counter: FxHashMap<usize, u64>,
}

use serde::Serialize;
use serde_derive::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
struct CategoryCnt {
    cat: String,
    cnt: u64,
}

impl From<(category_t, u64)> for CategoryCnt {
    fn from(value: (category_t, u64)) -> Self {
        Self {
            cat: format!("{:?}", value.0),
            cnt: value.1,
        }
    }
}

#[derive(Serialize, Deserialize)]
struct TaskCnt {
    task: u64,
    cnt: u64,
}

impl From<(TaskId, u64)> for TaskCnt {
    fn from(value: (TaskId, u64)) -> Self {
        Self {
            task: u64::from(value.0),
            cnt: value.1,
        }
    }
}

#[derive(Serialize, Deserialize)]
struct LineCnt {
    line: String,
    cnt: u64,
}

impl From<(usize, u64)> for LineCnt {
    fn from(value: (usize, u64)) -> Self {
        let mut addr: map_address_t = map_address_t {
            name: [0; 4096],
            offset: 0,
        };
        let addr_ptr: *mut map_address_t = &mut addr as *mut map_address_t;
        assert!(!addr_ptr.is_null());
        // Safety: We ensure that we are passing a valid, not null pointer
        // and trust the user to have file names shorter then 4kb.
        unsafe {
            memory_map_address_lookup(value.0, addr_ptr);
        }
        assert!(!addr_ptr.is_null());
        // Safety: We ensure the address is not null, and trust the user
        // to pass a valid C-string, with a 0 at the end.
        let addr_ptr = &mut addr.name as *mut i8;
        unsafe {
            LineCnt {
                line: format!("{:?} {:?}", CStr::from_ptr(addr_ptr), addr.offset),
                cnt: value.1,
            }
        }
    }
}

use std::ffi::CStr;
use std::fs;

impl PerfHandler {
    fn update_counts(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        // count of the task
        let cur = self.count_events_for_task.entry(tid).or_insert(0);
        *cur += 1;

        // count of the category
        let cur = self.count_category.entry(ctx_info.cat).or_insert(0);
        *cur += 1;

        // count of the program counter (pc)
        let cur = self.count_program_counter.entry(ctx_info.pc).or_insert(0);
        *cur += 1;
    }

    fn count_to_json<T, U>(&self, count_map: FxHashMap<T, u64>, name: String) -> String
    where
        U: From<(T, u64)>,
        U: Serialize,
    {
        let mut counts: Vec<(T, u64)> = count_map.into_iter().collect();
        // sort in non-increasing order by count
        counts.sort_by(|a, b| b.1.cmp(&a.1));

        let vec: Vec<U> = counts.into_iter().map(U::from).collect();
        format!("\"{name}\": ") + &serde_json::to_string(&vec).expect("valid json")
    }

    fn count_category_to_json(&self, count_map: FxHashMap<category_t, u64>) -> String {
        self.count_to_json::<category_t, CategoryCnt>(count_map, "count_per_category".to_string())
    }

    fn count_task_to_json(&self, count_map: FxHashMap<TaskId, u64>) -> String {
        self.count_to_json::<TaskId, TaskCnt>(count_map, "count_per_task".to_string())
    }

    fn count_lines_to_json(&self, count_map: FxHashMap<usize, u64>) -> String {
        self.count_to_json::<usize, LineCnt>(count_map, "count_per_line".to_string())
    }

    fn write_perf_log(&self) -> std::io::Result<()> {
        let cnt = self.count_category_to_json(self.count_category.clone());
        let events_for_task = self.count_task_to_json(self.count_events_for_task.clone());
        let map_pc_string = self.count_lines_to_json(self.count_program_counter.clone());
        let log =
            "{\n".to_string() + &cnt + ",\n" + &events_for_task + ",\n" + &map_pc_string + "\n}\n";
        trace!("\n{log}\n");

        fs::write("lotto-perf.json", log)?;
        Ok(())
    }
}

use std::env;

pub fn register() {
    let var_name = "LOTTO_PERF";
    match env::var(var_name) {
        Ok(v) => {
            if v == "true" {
                trace!("Registering lotto-perf handler, written in rust");
                lotto::engine::pubsub::subscribe_execute(&*HANDLER);
                return;
            }
        }
        Err(_) => {
            // not set
        }
    }
    trace!("Lotto-perf not set. To use it, do `export LOTTO_PERF=true`. Read the README for more details");
}

impl ExecuteHandler for PerfHandler {
    fn handle_execute(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        self.update_counts(tid, ctx_info);
        if tid == TaskId::new(1) && ctx_info.cat == base_category::CAT_FUNC_EXIT {
            // This can be the last event, so we can overwrite
            // the perf-log now, and maybe we will also need to do it later.
            // Note that the log is only written for successful executions
            // otherwise, why would you perf a erroneous code?
            let result = self.write_perf_log();
            match result {
                Ok(()) => {
                    debug!("Perf written into `lotto-perf.json`");
                }
                Err(_) => {
                    warn!("Error when writing data");
                }
            }
        }
    }
}
