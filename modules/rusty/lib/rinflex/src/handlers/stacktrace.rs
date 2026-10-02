use crate::{Either, StackFrameId, StackTrace};
use lotto::base::CapturePoint;
use lotto::collections::FxHashMap;
use lotto::sync::HandlerWrapper;
use lotto::{base::StableAddress, raw, Stateful};
use lotto::{
    base::{TaskId, Value},
    brokers::statemgr::*,
    cli::flags::{FlagKey, STR_CONVERTER_BOOL},
    engine::handler,
    log::*,
};
use std::ffi::{c_void, CStr};
use std::mem::MaybeUninit;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

pub static HANDLER: HandlerWrapper<StackTraceHandler> = HandlerWrapper::new(|| StackTraceHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    tasks: FxHashMap::default(),
    cache: FxHashMap::default(),
});

#[derive(Stateful)]
pub struct StackTraceHandler {
    #[config]
    pub cfg: Config,

    cache: FxHashMap<usize, StackFrameId>,
    tasks: FxHashMap<TaskId, TaskStack>,
}

#[derive(Default)]
struct TaskStack {
    pcs: Vec<usize>,
    raw_snapshot: Option<Arc<[usize]>>,
    resolved: Option<StackTrace>,
}

impl TaskStack {
    fn push(&mut self, pc: usize) {
        self.pcs.push(pc);
        self.raw_snapshot = None;
        self.resolved = None;
    }

    fn pop(&mut self) {
        self.pcs
            .pop()
            .expect("How can you exit a function before entering any?");
        self.raw_snapshot = None;
        self.resolved = None;
    }

    fn snapshot(&mut self, resolve: impl FnMut(usize) -> StackFrameId) -> StackTrace {
        self.resolved
            .get_or_insert_with(|| StackTrace(self.pcs.iter().copied().map(resolve).collect()))
            .clone()
    }
}

impl handler::Handler for StackTraceHandler {
    fn handle(&mut self, ctx: &CapturePoint, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let id = TaskId(ctx.id);
        match ctx.type_id as u32 {
            raw::EVENT_STACKTRACE_ENTER => {
                let caller_pc = ctx.caller_pc().expect("missing stacktrace caller");
                self.tasks.entry(id).or_default().push(caller_pc);
            }
            raw::EVENT_STACKTRACE_EXIT => {
                if let Some(stacktrace) = self.tasks.get_mut(&id) {
                    stacktrace.pop();
                }
            }
            _ => {}
        }
    }
}

/// Information about a PC.
///
/// - The first element is dli_sname, or offset if it's unavailable.
/// - The second element is dli_fname
type PCInfo = (Either<String, u64>, String);

#[repr(C)]
#[allow(non_camel_case_types)]
struct link_map {
    #[cfg(target_pointer_width = "64")]
    l_addr: libc::Elf64_Addr,
    #[cfg(target_pointer_width = "32")]
    l_addr: libc::Elf32_Addr,
}

fn get_pc_info1(pc: *const c_void) -> PCInfo {
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
    c"rusty-stacktrace",
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

/// Get stacktrace PCs without resolving stacktrace.
pub fn get_task_stacktrace_pcs(task: TaskId) -> Option<Arc<[usize]>> {
    let handler = unsafe { HANDLER.get_mut() };
    let stack = handler.tasks.get_mut(&task)?;
    Some(
        stack
            .raw_snapshot
            .get_or_insert_with(|| Arc::from(stack.pcs.as_slice()))
            .clone(),
    )
}

pub fn get_task_stacktrace(task: TaskId) -> Option<StackTrace> {
    let handler = unsafe { HANDLER.get_mut() };
    let stack = handler.tasks.get_mut(&task)?;
    Some(stack.snapshot(|pc| {
        handler
            .cache
            .entry(pc)
            .or_insert_with(|| {
                let (sname, fname) = get_pc_info1(pc.saturating_sub(1) as *const c_void);
                StackFrameId {
                    caller_pc: StableAddress::with_default_method(pc),
                    sname,
                    fname,
                }
            })
            .clone()
    }))
}

#[cfg(test)]
mod tests {
    extern crate lotto_link;

    use super::*;
    use std::sync::Arc;

    #[test]
    fn lazy_stack_snapshots_preserve_frames_and_wire_format() {
        let mut stack = TaskStack::default();
        stack.push(1);
        stack.push(2);
        stack.pop();
        assert!(stack.resolved.is_none());

        let mut resolved = Vec::new();
        let mut resolve = |pc| {
            resolved.push(pc);
            StackFrameId {
                caller_pc: StableAddress::with_method(
                    pc,
                    raw::stable_address_method::STABLE_ADDRESS_METHOD_MASK,
                ),
                sname: Either::Right(pc as u64),
                fname: "test".into(),
            }
        };
        let first = stack.snapshot(&mut resolve);
        let cached = stack.snapshot(|_| panic!("unchanged stack must use cached frames"));
        assert!(Arc::ptr_eq(&first.0, &cached.0));

        stack.push(3);
        let nested = stack.snapshot(&mut resolve);
        stack.pop();
        stack.pop();
        assert!(stack.snapshot(|_| unreachable!()).0.is_empty());
        assert_eq!(first.0.len(), 1);
        assert_eq!(nested.0.len(), 2);
        assert_eq!(resolved, [1, 1, 3]);

        let config = bincode::config::standard();
        let bytes = bincode::encode_to_vec(&nested, config).unwrap();
        assert_eq!(
            bytes,
            bincode::encode_to_vec(nested.0.to_vec(), config).unwrap()
        );
        let (decoded, used): (StackTrace, _) = bincode::decode_from_slice(&bytes, config).unwrap();
        assert_eq!(used, bytes.len());
        assert_eq!(decoded, nested);
    }
}
