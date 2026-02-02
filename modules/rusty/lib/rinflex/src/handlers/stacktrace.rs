use crate::{StackFrameId, StackTrace};
use lotto::collections::FxHashMap;
use lotto::{base::HandlerArg, base::StableAddress, raw, Stateful};
use lotto::{
    base::Value,
    brokers::statemgr::*,
    cli::flags::{FlagKey, STR_CONVERTER_BOOL},
    engine::handler::{self, TaskId},
    log::*,
};
use std::ffi::CStr;
use std::sync::{
    atomic::{AtomicBool, Ordering},
    LazyLock,
};

pub static HANDLER: LazyLock<StackTraceHandler> = LazyLock::new(|| StackTraceHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    tasks: FxHashMap::default(),
});

#[derive(Stateful)]
pub struct StackTraceHandler {
    #[config]
    pub cfg: Config,

    pub tasks: FxHashMap<TaskId, StackTrace>,
}

impl handler::Handler for StackTraceHandler {
    fn handle(&mut self, ctx: &raw::context_t, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let id = TaskId(ctx.id);
        match ctx.cat {
            raw::base_category::CAT_FUNC_ENTRY => {
                // CAVEAT: if we don't have "dli_sname", we try to get
                // (call_return_address - fbase) as the identifier.
                // But this may incorrectly give a function multiple
                // identifiers, e.g.:
                //
                // foo [static]:
                //     CALL bar
                //     CALL baz
                //
                // If in bar and baz we try to find the foo's id, we
                // will get two different ids due to different return
                // addresses.
                //
                // This is not an issue in our case: we only try to
                // retrieve function id from tsan's func_entry and
                // func_exit, and each function is guaranteed to have
                // only one pair of them.
                let call_pc = get_arg_ptr(&ctx.args[0]) - call_insn_len();
                let call_pc = StableAddress::with_default_method(call_pc);
                let sname = get_arg_str(&ctx.args[1]);
                let fname = get_arg_str(&ctx.args[2]).unwrap_or("UNKNOWN".to_string());
                let offset = get_arg_ptr(&ctx.args[3]);
                let frame_id = StackFrameId {
                    call_pc,
                    sname,
                    fname,
                    offset,
                };
                self.tasks
                    .entry(id)
                    .and_modify(|trace| trace.0.push(frame_id.clone()))
                    .or_insert(StackTrace(vec![frame_id]));
            }
            raw::base_category::CAT_FUNC_EXIT => {
                if let Some(stacktrace) = self.tasks.get_mut(&id) {
                    stacktrace
                        .0
                        .pop()
                        .expect("How can you exit a function before entering any?");
                }
            }
            _ => {}
        }
    }
}

fn get_arg_ptr(arg: &raw::arg) -> usize {
    let arg = HandlerArg::from(*arg);
    arg.addr()
}

fn get_arg_str(arg: &raw::arg) -> Option<String> {
    let arg = HandlerArg::from(*arg);
    let ptr = arg.addr() as *const i8;
    if ptr.is_null() {
        return None;
    }
    let cstr = unsafe { CStr::from_ptr(ptr) };
    let s = cstr.to_str().unwrap();
    Some(s.to_string())
}

/// Length of a function call instruction.
fn call_insn_len() -> usize {
    if cfg!(target_arch = "x86_64") {
        5 // FIXME: This is incomplete.
    } else if cfg!(target_arch = "aarch64") {
        4
    } else {
        0
    }
}

//
// States
//

#[derive(Encode, Decode, Debug)]
pub struct Config {
    pub enabled: AtomicBool,
}

impl Marshable for Config {
    fn print(&self) {
        info!("enabled = {}", self.enabled.load(Ordering::Relaxed));
    }
}

pub static FLAG_HANDLER_STACKTRACE_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_STACKTRACE_ENABLED",
    c"",
    c"handler-stacktrace",
    c"",
    c"enable the stacktrace handler",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_stacktrace_enabled_callback),
);

unsafe extern "C" fn _handler_stacktrace_enabled_callback(v: raw::value, _: *mut std::ffi::c_void) {
    let enabled = Value::from(v).is_on();
    HANDLER.cfg.enabled.store(enabled, Ordering::Relaxed);
}

//
// Registration
//
pub fn register() {
    lotto::engine::handler::register(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
}

pub fn register_flags() {
    let _ = FLAG_HANDLER_STACKTRACE_ENABLED.get();
}

//
// Interfaces
//

pub fn get_task_stacktrace(task: TaskId) -> Option<StackTrace> {
    HANDLER.tasks.get(&task).cloned()
}
