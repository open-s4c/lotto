use lotto::base::*;
use lotto::cli::flags::*;
use lotto::cli::*;
use lotto::engine::flags::*;
use lotto::*;
use rinflex::handlers::flags::*;

#[subcmd(
    name="replay-rs",
    args="",
    desc="replay in rust",
    engine_flags=true,
    cli_flags=[
        FLAG_INPUT.get(), FLAG_OUTPUT.get(), FLAG_VERBOSE.get(), FLAG_REPLAY_GOAL.get(),
        FLAG_TEMPORARY_DIRECTORY.get(), FLAG_NO_PRELOAD.get(), FLAG_LOGGER_BLOCK.get(), FLAG_BEFORE_RUN.get(), FLAG_AFTER_RUN.get()
    ],
    defaults=|flags: &mut Flags| {
        flags.set_default(&flag_stable_address_method(),
                          Value::U64(StableAddressMethod::STABLE_ADDRESS_METHOD_MAP as u64));
        flags.set_default(&FLAG_HANDLER_ADDRESS_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_CAS_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_EVENT_ENABLED, Value::Bool(true));
        flags.set_default(&FLAG_HANDLER_STACKTRACE_ENABLED, Value::Bool(true));
    },
    group=raw::subcmd_group::SUBCMD_GROUP_TRACE
)]
fn replay(_args: &mut Args, flags: &mut Flags) -> SubCmdResult {
    envvar_set! {
        "LOTTO_LOGGER_FILE" => flags.get_sval(&FLAG_LOGGER_FILE),
    };

    println!("trace file: {}", flags.get_sval(&FLAG_INPUT));

    let last_clk = cli_trace_last_clk(flags.get_sval(&FLAG_INPUT));
    let goal = flags.get_uval(&FLAG_REPLAY_GOAL);
    if goal > last_clk {
        flags.set_default(&FLAG_REPLAY_GOAL, Value::U64(last_clk));
    }

    let input_fn = flags.get_sval(&FLAG_INPUT);
    let mut rec = Trace::load_file(input_fn);
    let first: &Record = match rec.next(raw::record::RECORD_START) {
        None => {
            eprintln!("error: empty trace file '{}'", input_fn);
            return Err(1.into());
        }
        Some(first) => first,
    };

    let args = first.args().to_owned();
    rec.advance();
    rec.next(raw::record::RECORD_CONFIG).unwrap().unmarshal();
    drop(rec);

    envvar_set! {
        "LOTTO_REPLAY" => flags.get_sval(&FLAG_INPUT),
        "LOTTO_LOGGER_BLOCK" => flags.get_sval(&FLAG_LOGGER_BLOCK),
    };

    let output_fn = flags.get_sval(&FLAG_OUTPUT);
    envvar_set! {
        "LOTTO_RECORD" => output_fn,
    };

    let record_granularity: RecordGranularities = flags
        .get_uval(unsafe { &raw::flag_record_granularity() })
        .into();
    envvar_set! {
        "LOTTO_RECORD_GRANULARITY" => record_granularity.to_string()
    };

    println!("[lotto] replaying: {}", flags.get_sval(&FLAG_INPUT));
    println!("[lotto] recording: {}", flags.get_sval(&FLAG_OUTPUT));

    preload(
        flags.get_sval(&FLAG_TEMPORARY_DIRECTORY),
        flags.is_on(&FLAG_VERBOSE),
        !flags.is_on(&FLAG_NO_PRELOAD),
        flags.get_sval(&flag_memmgr_runtime()),
        flags.get_sval(&flag_memmgr_user()),
    );

    adjust(flags.get_sval(&FLAG_INPUT));
    let exitcode = execute(&args, flags, true);

    if cfg!(feature = "stable_address_map") {
        use rinflex::handlers::order_enforcer;
        order_enforcer::cli_reset();
        adjust(flags.get_sval(&FLAG_OUTPUT));
        if order_enforcer::should_discard() {
            println!(
                "number of constraints: {}",
                order_enforcer::HANDLER.fin.constraints.len()
            );
            println!(
                "should_discard = {}",
                order_enforcer::HANDLER.fin.should_discard
            );
            println!("** INVALID **");
        } else {
            println!("valid");
        }
    }
    SubCmdResult::from_error_code(exitcode)
}
