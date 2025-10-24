use std::path::PathBuf;

use lotto::base::*;
use lotto::cli::flags::*;
use lotto::cli::*;
use lotto::engine::flags::*;
use lotto::log::*;
use lotto::*;
use rinflex::error::Error;

#[lotto::subcmd(
    name="inflex-rs",
    args="",
    desc="inflex in rust",
    engine_flags=true,
    cli_flags=[
        FLAG_INPUT.get(),
        FLAG_OUTPUT.get(),
        FLAG_VERBOSE.get(),
        FLAG_ROUNDS.get(),
        FLAG_TEMPORARY_DIRECTORY.get(),
        FLAG_NO_PRELOAD.get(),
        FLAG_LOGGER_BLOCK.get(),
        FLAG_CREP.get(),
        FLAG_BEFORE_RUN.get(),
        FLAG_AFTER_RUN.get(),
        // FLAG_INFLEX_MIN.get(),
        // FLAG_INFLEX_METHOD.get(),
        FLAG_LOG_FILE.get()
    ],
    group=raw::subcmd_group::SUBCMD_GROUP_TRACE,
    defaults=|flags: &mut Flags| {
        flags.set_default(&flag_strategy(), Value::Sval(c"random"));
    },
)]
fn inflex(_args: &mut Args, flags: &mut Flags) -> SubCmdResult {
    envvar_set!("LOTTO_LOG_FILE" => flags.get_sval(&FLAG_LOG_FILE));

    let input = flags.get_sval(&FLAG_INPUT).to_string();
    let output = flags.get_sval(&FLAG_INPUT).to_string();
    let tempdir = PathBuf::from(flags.get_sval(&FLAG_TEMPORARY_DIRECTORY));
    println!("input trace: {}", input);
    println!("output trace: {}", output);

    let inflex = rinflex::inflex::Inflex::new_with_flags(flags);

    preload(
        &tempdir,
        flags.is_on(&FLAG_VERBOSE),
        !flags.is_on(&FLAG_NO_PRELOAD),
        flags.is_on(&FLAG_CREP),
        false,
        flags.get_sval(&flag_memmgr_runtime()),
        flags.get_sval(&flag_memmgr_user()),
    );

    match inflex.run_fast() {
        Ok(ip) => {
            info!("inflection point = {}", ip);
            let mut rec = Trace::load_file(&input);
            rec.truncate(output, ip);
            Ok(())
        }
        Err(Error::Interrupted) => Ok(()),
        Err(Error::LottoError) => {
            error!("Inflex failed due to Lotto internal errors");
            Ok(())
        }
        Err(e @ _) => unreachable!("Error {:?} should not be reached", e),
    }
}
