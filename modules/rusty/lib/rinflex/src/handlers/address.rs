use lotto::base::CapturePoint;
use lotto::collections::FxHashMap;
use lotto::{
    base::{StableAddress, TaskId, Value},
    brokers::statemgr::*,
    cli::{flags::STR_CONVERTER_BOOL, FlagKey},
    engine::handler,
    log::*,
};
use lotto::{raw, Stateful};
use std::collections::{BTreeMap, BTreeSet};
use std::sync::{
    atomic::{AtomicBool, Ordering},
    LazyLock,
};

static HANDLER: LazyLock<AddressHandler> = LazyLock::new(|| AddressHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    pers: Persistent {
        tasks: FxHashMap::default(),
    },
    task_origin: FxHashMap::default(),
    eclass: EquivClass::new(),
});

#[derive(Stateful)]
struct AddressHandler {
    #[config]
    pub cfg: Config,
    #[persistent]
    pub pers: Persistent,

    pub task_origin: FxHashMap<TaskId, TaskOrigin>,
    pub eclass: EquivClass<TaskOrigin, TaskId>,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub enum TaskOrigin {
    Main,
    Child { run: usize, arg: usize },
}

impl handler::Handler for AddressHandler {
    fn handle(&mut self, ctx: &CapturePoint, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        self.update_task_pc(ctx);
        self.update_task_eclass(ctx);
    }
}

impl AddressHandler {
    fn update_task_pc(&mut self, ctx: &CapturePoint) {
        let tasks = &mut self.pers.tasks;
        let addr = StableAddress::with_default_method(ctx.pc);
        let info = AddressInfo {
            addr,
            type_id: ctx.type_id as u32,
            after: u32::from(ctx.chain_id) == raw::CHAIN_INGRESS_AFTER,
        };
        tasks.insert(TaskId::new(ctx.id), info);
    }

    fn update_task_eclass(&mut self, ctx: &CapturePoint) {
        match ctx.type_id as u32 {
            raw::EVENT_TASK_INIT if ctx.id != 1 => {
                let (run, arg) = unsafe {
                    let payload: &raw::capture_task_init_event = ctx
                        .payload_unchecked()
                        .expect("TASK_INIT should have payload");
                    (payload.run, payload.arg)
                };
                let tid = TaskId(ctx.id);
                let origin = TaskOrigin::Child { run, arg };
                self.task_origin.insert(tid, origin);
                self.eclass.insert(origin, tid);
            }
            raw::EVENT_TASK_FINI if ctx.id != 1 => {
                let tid = TaskId(ctx.id);
                let origin = self.task_origin.remove(&tid).unwrap();
                self.eclass.remove(&origin, &tid);
            }
            _ => {}
        }
    }
}

/// A class of `Elem` are considered equivalent if their `Key`s are equal.
#[derive(Default)]
struct EquivClass<Key, Elem> {
    // Invariant: class[key] is not empty <=> representative[key] is not None
    class: BTreeMap<Key, BTreeSet<Elem>>,

    /// The first inserted element for the key.  It is stored until
    /// the eclass is removed, even if the elem itself is removed.
    representative: BTreeMap<Key, Elem>,
}

impl<Key: Clone + Ord, Elem: Clone + Ord> EquivClass<Key, Elem> {
    fn new() -> EquivClass<Key, Elem> {
        EquivClass {
            class: BTreeMap::new(),
            representative: BTreeMap::new(),
        }
    }

    fn insert(&mut self, key: Key, elem: Elem) {
        if self.class.contains_key(&key) {
            let group = self.class.get_mut(&key).unwrap();
            group.insert(elem);
        } else {
            // Representative
            let rep = elem.clone();
            self.representative.insert(key.clone(), rep);
            // Group
            let mut group = BTreeSet::new();
            group.insert(elem);
            self.class.insert(key, group);
        }
    }

    fn remove(&mut self, key: &Key, elem: &Elem) {
        let Some(group) = self.class.get_mut(key) else {
            return;
        };
        group.remove(elem);
        if !group.is_empty() {
            return;
        }
        // group is empty. remove this class.
        self.class.remove(key);
        self.representative.remove(key);
    }

    fn rep_from_key(&self, key: &Key) -> Option<&Elem> {
        self.representative.get(key)
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

#[derive(Encode, Decode, Debug, Clone, MarshableNoPrint)]
pub struct AddressInfo {
    pub addr: StableAddress,
    pub type_id: u32,
    pub after: bool,
}

#[derive(Encode, Decode, Debug)]
pub struct Persistent {
    pub tasks: FxHashMap<TaskId, AddressInfo>,
}

impl Marshable for Persistent {
    fn print(&self) {
        for (id, info) in self.tasks.iter() {
            info!(
                "task {} {} type={} after={}",
                id, info.addr, info.type_id, info.after
            );
        }
    }
}

pub static FLAG_HANDLER_ADDRESS_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_ADDRESS_ENABLED",
    c"",
    c"rusty-address",
    c"",
    c"enable the address handler",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_address_enabled_callback),
);

unsafe extern "C" fn _handler_address_enabled_callback(v: raw::value, _: *mut std::ffi::c_void) {
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
    let _ = FLAG_HANDLER_ADDRESS_ENABLED.get();
}

//
// Interfaces
//

pub fn get_task_address(task: TaskId) -> AddressInfo {
    HANDLER
        .pers
        .tasks
        .get(&task)
        .expect("Task address information not available; have you enabled rusty-address?")
        .clone()
}

pub fn get_task_origin(task: TaskId) -> Option<TaskOrigin> {
    if task.0 == 1 {
        return Some(TaskOrigin::Main);
    }
    HANDLER.task_origin.get(&task).cloned()
}

pub fn get_task_eid(task: TaskId) -> Option<TaskId> {
    if task.0 == 1 {
        return Some(task);
    }
    let origin = get_task_origin(task)?;
    HANDLER.eclass.rep_from_key(&origin).cloned()
}
