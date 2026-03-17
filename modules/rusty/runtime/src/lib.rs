use std::sync::Once;

#[global_allocator]
static GLOBAL: loccolator::LottoAllocator = loccolator::LottoAllocator;

static SUBSCRIBE_ONCE: Once = Once::new();
static REGISTER_ONCE: Once = Once::new();
static INIT_ONCE: Once = Once::new();

/// Phase 0: install Dice subscriptions required by Rust runtime support.
pub fn rusty_subscribe() {
    SUBSCRIBE_ONCE.call_once(|| {
        lotto::log::init();
    });
}

/// Phase 1: register Rust runtime state and handlers.
pub fn rusty_register() {
    REGISTER_ONCE.call_once(|| {
        lotto::log::init();
        rusty_subscribe();
        lotto::brokers::statemgr::register_system();
        lotto::engine::pubsub::register_phase();
        rusty_handlers::register();
    });
}

/// Phase 2: perform Rust runtime initialization after registration.
pub fn rusty_init() {
    INIT_ONCE.call_once(|| {
        lotto::log::init();
        lotto::engine::pubsub::initialization_phase();
    });
}

#[no_mangle]
pub extern "C" fn lotto_rust_subscribe() {
    rusty_subscribe();
}

#[no_mangle]
pub extern "C" fn lotto_rust_register() {
    rusty_register();
}

#[no_mangle]
pub extern "C" fn lotto_rust_init() {
    rusty_init();
}

#[no_mangle]
pub unsafe extern "C" fn lotto_rust_publish_arrival(
    ctx: *const lotto::raw::context_t,
    event: *mut lotto::raw::event_t,
) {
    lotto::engine::pubsub::publish_arrival(ctx, event);
}

#[no_mangle]
pub unsafe extern "C" fn lotto_rust_publish_execute(v: *const lotto::raw::value) {
    lotto::engine::pubsub::publish_execute_value(*v);
}

#[no_mangle]
pub extern "C" fn lotto_rust_after_unmarshal_config() {
    lotto::brokers::statemgr::after_unmarshal_config();
}

#[no_mangle]
pub extern "C" fn lotto_rust_after_unmarshal_persistent() {
    lotto::brokers::statemgr::after_unmarshal_persistent();
}

#[no_mangle]
pub extern "C" fn lotto_rust_after_unmarshal_final() {
    lotto::brokers::statemgr::after_unmarshal_final();
}

#[no_mangle]
pub extern "C" fn lotto_rust_before_marshal_config() {
    lotto::brokers::statemgr::before_marshal_config();
}

#[no_mangle]
pub extern "C" fn lotto_rust_before_marshal_persistent() {
    lotto::brokers::statemgr::before_marshal_persistent();
}

#[no_mangle]
pub extern "C" fn lotto_rust_before_marshal_final() {
    lotto::brokers::statemgr::before_marshal_final();
}

#[no_mangle]
pub extern "C" fn lotto_rust_engine_init() {
    // Legacy entry point kept as an alias for the registration phase.
    rusty_register();
}

pub fn init() {
    // Legacy Rust-only entry point kept as an alias for the registration phase.
    rusty_register();
}
