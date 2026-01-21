use lotto::base::{Flags, StableAddressMethod, Value};
use lotto::cli::flags::*;
use lotto::cli::Args;
use lotto::cli::SubCmdResult;
use lotto::engine::flags::*;
use lotto::log::*;
use lotto::*;

use rec_inflex::{Outcome, RecInflex};
use rinflex::error::Error;
use rinflex::handlers::flags::*;
use rinflex::*;

#[lotto::subcmd(
    name="rinflex",
    args="",
    desc="Find a minimal set of ordering constraints for a bug to manifest",
    engine_flags=true,
    cli_flags=[
        FLAG_INPUT.get(),
        FLAG_OUTPUT.get(),
        FLAG_VERBOSE.get(),
        FLAG_ROUNDS.get(),
        FLAG_TEMPORARY_DIRECTORY.get(),
        FLAG_NO_PRELOAD.get(),
        FLAG_LOGGER_BLOCK.get(),
    ],
    group=raw::subcmd_group::SUBCMD_GROUP_TRACE,
    defaults=|flags: &mut Flags| {
        flags.set_default(&flag_strategy(), Value::Sval(c"random"));
        flags.set_default(&flag_stable_address_method(),
                          Value::U64(StableAddressMethod::STABLE_ADDRESS_METHOD_MAP as u64));
        flags.set_default(&FLAG_HANDLER_ADDRESS_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_CAS_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_EVENT_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_STACKTRACE_ENABLED, Value::Bool(true));
    }
)]
fn main(args: &mut Args, flags: &mut Flags) -> SubCmdResult {
    main1(args, flags).inspect_err(|e| {
        eprintln!("Error: {}", e);
    })?;
    Ok(())
}

fn main1(_args: &mut Args, flags: &mut Flags) -> Result<(), rinflex::error::Error> {
    envvar_set! {
        "LOTTO_MODIFY_RETURN_CODE" => 1 as u64,
        "LOTTO_LOGGER_BLOCK" => flags.get_sval(&FLAG_LOGGER_BLOCK),
    };

    info!("rinflex trace: {}", flags.get_sval(&FLAG_INPUT));

    let mut rinflex = RecInflex::new_with_flags(&flags);

    cli::preload(
        flags.get_sval(&FLAG_TEMPORARY_DIRECTORY),
        flags.is_on(&FLAG_VERBOSE),
        !flags.is_on(&FLAG_NO_PRELOAD),
        false,
        flags.get_sval(&flag_memmgr_runtime()),
        flags.get_sval(&flag_memmgr_user()),
    );

    loop {
        println!(
            "Currently there are {} constraints",
            rinflex.constraints.len()
        );
        for (i, c) in rinflex.constraints.iter().enumerate() {
            println!(
                "=== Constraint {}{}",
                i + 1,
                if c.virt { " [virtual]" } else { "" }
            );
            println!("{}", c.c.display().unwrap());
        }

        if rinflex.should_terminate()? {
            println!("The current constraint set is sufficient to reproduce the bug, stopping");
            break;
        } else {
            println!("We still need more constraints...");
        }
        let pair = match rinflex.find_next_pair() {
            Ok(pair) => pair,
            Err(Error::ExecutionNotFound) => {
                println!("Cannot find an execution that satisfied the given constraints. This is likely due to circular constraints or control dependence.");
                break;
            }
            Err(e) => return Err(e),
        };

        info!("Checking if the constraint is essential...");
        let essential = rinflex.check_if_essential(&pair)?;
        let constraint = if essential {
            println!("needed");
            Constraint {
                c: pair,
                virt: false,
                positive: true,
            }
        } else {
            println!("not needed");
            Constraint {
                c: pair.flipped(),
                virt: true,
                positive: false,
            }
        };
        rinflex.constraints.push(constraint);

        // [v] even though we conceptually want to find an
        // unrestricted failing execution from clock 0, we actually
        // start from pair.clock_lower_bound. This is under the
        // assumption that the more we replay the prefix, the closer
        // we get to the bug.
        match rinflex.get_trace_from_zero(
            Outcome::Fail,
            &rinflex.trace_success,
            &rinflex.trace_fail,
        ) {
            Ok(_) => {}
            Err(Error::ExecutionNotFound) => {
                println!("Cannot find an execution that satisfied the given constraints. This is likely due to circular constraints or control dependence.");
                break;
            }
            e @ Err(_) => return e,
        }
        println!("OK, found another failing trace!");
    }

    for c in &rinflex.constraints {
        println!(
            "------ . ------ . ------ . ------ . ------ {}",
            if c.virt { "[virtual]" } else { "" }
        );
        println!("{}", c.c.display().unwrap());
    }

    Ok(())
}
