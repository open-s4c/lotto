use crate::{Either, StackFrameId, StackTrace};
use lotto::collections::FxHashMap;
use lotto::{base::HandlerArg, base::StableAddress, raw, Stateful};
use lotto::{
    base::Value,
    brokers::statemgr::*,
    cli::flags::{FlagKey, STR_CONVERTER_BOOL},
    engine::handler::{self, TaskId},
    log::*,
};
use std::ffi::{c_void, CStr};
use std::mem::MaybeUninit;
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
                let caller_pc = get_arg_ptr(&ctx.args[0]) - call_insn_len();
                let (sname, fname) = get_pc_info(caller_pc as *const c_void);
                let caller_pc = StableAddress::with_default_method(caller_pc);
                let frame_id = StackFrameId {
                    caller_pc,
                    sname,
                    fname,
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

#[repr(C)]
#[allow(non_camel_case_types)]
struct link_map {
    #[cfg(target_pointer_width = "64")]
    l_addr: libc::Elf64_Addr,
    #[cfg(target_pointer_width = "32")]
    l_addr: libc::Elf32_Addr,
}

/// Given a PC, find its symbol and the file.
fn get_pc_info(pc: *const c_void) -> (Either<String, u64>, String) {
    unsafe {
        let mut info = MaybeUninit::uninit();
        let mut map = MaybeUninit::uninit();
        libc::dladdr1(
            pc,
            info.as_mut_ptr(),
            map.as_mut_ptr(),
            libc::RTLD_DI_LINKMAP,
        );
        let info = info.assume_init();

        let sname = if !info.dli_sname.is_null() {
            Either::Left(CStr::from_ptr(info.dli_sname).to_string_lossy().to_string())
        } else {
            // Get the offset of the CALL instruction into the ELF file
            let map = map.assume_init() as *mut link_map;
            let map = &*map;
            let pc = pc as u64;
            Either::Right(pc - map.l_addr)
        };
        let fname = CStr::from_ptr(info.dli_fname).to_string_lossy().to_string();
        (sname, fname)
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
