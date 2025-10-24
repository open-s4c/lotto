use lotto::collections::FxHashMap;
use lotto::{
    base::category::Category,
    base::StableAddress,
    base::Value,
    brokers::statemgr::*,
    cli::{flags::STR_CONVERTER_BOOL, FlagKey},
    engine::handler::{self, TaskId},
    log::*,
};
use lotto::{raw, Stateful};
use std::sync::{
    atomic::{AtomicBool, Ordering},
    LazyLock,
};

pub static HANDLER: LazyLock<AddressHandler> = LazyLock::new(|| AddressHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    pers: Persistent {
        tasks: FxHashMap::default(),
    },
});

#[derive(Stateful)]
pub struct AddressHandler {
    #[config]
    pub cfg: Config,
    #[persistent]
    pub pers: Persistent,
}

impl handler::Handler for AddressHandler {
    fn handle(&mut self, ctx: &raw::context_t, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let tasks = &mut self.pers.tasks;
        let addr = StableAddress::with_default_method(ctx.pc);
        let cat = ctx.cat;
        let info = AddressInfo { addr, cat };
        tasks.insert(TaskId::new(ctx.id), info);
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
    pub cat: Category,
}

#[derive(Encode, Decode, Debug)]
pub struct Persistent {
    pub tasks: FxHashMap<TaskId, AddressInfo>,
}

impl Marshable for Persistent {
    fn print(&self) {
        for (id, info) in self.tasks.iter() {
            info!("task {} {} {}", id, info.addr, info.cat);
        }
    }
}

pub static FLAG_HANDLER_ADDRESS_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_ADDRESS_ENABLED",
    c"",
    c"handler-address",
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

pub fn get_task_address(task: TaskId) -> Option<AddressInfo> {
    HANDLER.pers.tasks.get(&task).cloned()
}
