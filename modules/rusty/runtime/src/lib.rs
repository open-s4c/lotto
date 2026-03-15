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
        lotto::brokers::statemgr::subscribe_system();
        lotto::engine::pubsub::subscribe_phase();
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
pub extern "C" fn lotto_rust_engine_init() {
    // Legacy entry point kept as an alias for the registration phase.
    rusty_register();
}

pub fn init() {
    // Legacy Rust-only entry point kept as an alias for the registration phase.
    rusty_register();
}
