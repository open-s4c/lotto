use std::sync::Once;

#[global_allocator]
static GLOBAL: loccolator::LottoAllocator = loccolator::LottoAllocator;

static REGISTER_ONCE: Once = Once::new();
static INIT_ONCE: Once = Once::new();

pub fn rusty_register() {
    REGISTER_ONCE.call_once(|| {
        lotto::log::init();
        lotto::brokers::statemgr::init();
        lotto::engine::pubsub::init();
        rusty_handlers::register();
    });
}

pub fn rusty_init() {
    INIT_ONCE.call_once(|| {
        lotto::log::init();
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
    rusty_register();
}

pub fn init() {
    rusty_register();
}
