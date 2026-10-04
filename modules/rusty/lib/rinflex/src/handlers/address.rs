use lotto::base::CapturePoint;
use lotto::collections::FxHashMap;
use lotto::{
    base::{StableAddress, StableAddressMethod, TaskId, Value},
    brokers::statemgr::*,
    cli::{flags::STR_CONVERTER_BOOL, FlagKey},
    engine::handler,
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
    cache: FxHashMap::default(),
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
    // Process-local: raw PCs must not be restored from a recorded execution.
    cache: FxHashMap<(StableAddressMethod, usize), StableAddress>,
}

impl handler::Handler for AddressHandler {
    fn handle(&mut self, ctx: &CapturePoint, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let tasks = &mut self.pers.tasks;
        let method = unsafe { (*raw::sequencer_config()).stable_address_method };
        let addr = self
            .cache
            .entry((method, ctx.pc))
            .or_insert_with(|| StableAddress::with_method(ctx.pc, method));
        let info = tasks
            .entry(TaskId::new(ctx.id))
            .or_insert_with(|| AddressInfo {
                addr: addr.clone(),
                type_id: 0,
                after: false,
            });
        info.addr.clone_from(addr);
        info.type_id = ctx.type_id as u32;
        info.after = u32::from(ctx.chain_id) == raw::CHAIN_INGRESS_AFTER;
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

pub fn get_task_address(task: TaskId) -> Option<AddressInfo> {
    HANDLER.pers.tasks.get(&task).cloned()
}

#[cfg(test)]
mod tests {
    use super::*;
    use lotto::engine::handler::Handler;

    #[test]
    fn cached_addresses_follow_method_and_current_event() {
        let mut handler = AddressHandler {
            cfg: Config {
                enabled: AtomicBool::new(true),
            },
            pers: Persistent {
                tasks: FxHashMap::default(),
            },
            cache: FxHashMap::default(),
        };
        let config = unsafe { raw::sequencer_config() };
        let saved_method = unsafe { (*config).stable_address_method };
        let mut event = unsafe { std::mem::zeroed() };
        for method in [
            raw::stable_address_method::STABLE_ADDRESS_METHOD_NONE,
            raw::stable_address_method::STABLE_ADDRESS_METHOD_MASK,
            raw::stable_address_method::STABLE_ADDRESS_METHOD_MAP,
            raw::stable_address_method::STABLE_ADDRESS_METHOD_NONE,
        ] {
            unsafe { (*config).stable_address_method = method };
            for (id, type_id, chain_id) in [
                (1, 10, 0),
                (2, 11, raw::CHAIN_INGRESS_AFTER),
                (1, 12, raw::CHAIN_INGRESS_AFTER),
            ] {
                let ctx = CapturePoint::from(raw::capture_point {
                    pc: cached_addresses_follow_method_and_current_event as *const () as usize,
                    id,
                    type_id,
                    chain_id: chain_id as _,
                    ..unsafe { std::mem::zeroed() }
                });
                handler.handle(&ctx, &mut event);
                let info = &handler.pers.tasks[&TaskId::new(id)];
                assert_eq!(info.addr, StableAddress::with_method(ctx.pc, method));
                assert_eq!(info.type_id, type_id as u32);
                assert_eq!(info.after, chain_id == raw::CHAIN_INGRESS_AFTER);
            }
            // Replay can replace persistent state while the local cache survives.
            handler.pers.tasks.clear();
        }
        unsafe { (*config).stable_address_method = saved_method };
        assert_eq!(handler.cache.len(), 3);
    }
}
