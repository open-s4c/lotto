use lotto::base::Flags;
use lotto::cli::{Args, SubCmdResult};

#[lotto::subcmd(name = "hello-rust", args = "", desc = "Greetings from the Crabland")]
fn hello_rust(args: &mut Args, _flags: &mut Flags) -> SubCmdResult {
    println!("Hello Rust!");
    for (i, arg) in args.strs().enumerate() {
        println!("args[{}]={}", i, arg);
    }
    Ok(())
}
