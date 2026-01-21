#[global_allocator]
static GLOBAL: loccolator::LottoAllocator = loccolator::LottoAllocator;

#[no_mangle]
pub extern "C" fn lotto_rust_engine_init() {
    init();
}

pub fn init() {
    lotto::log::init();
    lotto::brokers::statemgr::init();
    lotto::engine::pubsub::init();
    rusty_handlers::register();
}
