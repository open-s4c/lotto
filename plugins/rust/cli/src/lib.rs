//

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

#[no_mangle]
pub extern "C" fn lotto_rust_cli_init() {
    lotto::log::init();
    lotto::brokers::statemgr::init();
    lotto::cli::flags::init();
    rusty_handlers::register();
    rusty_handlers::register_flags();

    #[cfg(feature = "demo")]
    {
        cli_hello_rust::subcmd_init();
        cli_replay::subcmd_init();
        cli_inflex::subcmd_init();
    }

    #[cfg(feature = "stable_address_map")]
    cli_rinflex::subcmd_init();
}
