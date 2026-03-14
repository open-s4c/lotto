use std::sync::Once;

#[global_allocator]
static GLOBAL: loccolator::LottoAllocator = loccolator::LottoAllocator;

static REGISTER_ONCE: Once = Once::new();
static INIT_ONCE: Once = Once::new();

/// Phase 2: register Rust runtime state and handlers.
pub fn rusty_register() {
    REGISTER_ONCE.call_once(|| {
        lotto::log::init();
        lotto::brokers::statemgr::register_system();
        rusty_handlers::register();
    });
}

/// Phase 3: hook Rust runtime execution into Lotto after registration.
pub fn rusty_init() {
    INIT_ONCE.call_once(|| {
        lotto::log::init();
        lotto::engine::pubsub::init();
    });
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
