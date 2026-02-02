pub mod logger;
pub mod panic_hook;

pub fn init() {
    panic_hook::init();
    logger::init();
}
