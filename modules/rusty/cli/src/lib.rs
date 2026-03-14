//
use std::sync::Once;

#[cfg(feature = "demo")]
pub mod cli_hello_rust;
#[cfg(feature = "demo")]
pub mod cli_inflex;
#[cfg(feature = "demo")]
pub mod cli_replay;
#[cfg(feature = "stable_address_map")]
pub mod cli_rinflex;

#[global_allocator]
static GLOBAL: loccolator::LottoAllocator = loccolator::LottoAllocator;

static REGISTER_ONCE: Once = Once::new();
static INIT_ONCE: Once = Once::new();

/// Phase 2: register Rust CLI flags, categories, states, and command metadata.
pub fn rusty_register() {
    REGISTER_ONCE.call_once(|| {
        lotto::log::init();
        lotto::brokers::statemgr::register_system();

        // Handler registration touches custom categories through their
        // CategoryKey accessors, so it must happen before flags and commands.
        rusty_handlers::register();

        lotto::cli::flags::init();
        rusty_handlers::register_flags();
    });
}

/// Phase 3: install Rust CLI subcommands after registration is complete.
pub fn rusty_init() {
    INIT_ONCE.call_once(|| {
        lotto::log::init();

        #[cfg(feature = "demo")]
        {
            cli_hello_rust::subcmd_init();
            cli_replay::subcmd_init();
            cli_inflex::subcmd_init();
        }

        #[cfg(feature = "stable_address_map")]
        cli_rinflex::subcmd_init();
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
pub extern "C" fn lotto_rust_cli_register_flags() {
    // Legacy entry point kept as an alias for the registration phase.
    rusty_register();
}

#[no_mangle]
pub extern "C" fn lotto_rust_cli_init() {
    // Legacy entry point kept as an alias for the init phase.
    rusty_init();
}
