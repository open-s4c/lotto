use proc_macro::TokenStream;

mod brokers;
mod cli;
mod engine;

/// Declare a subcommand for Lotto CLI.
///
/// `#[lotto::subcmd(...)]` generates
///
/// 1. a `subcmd_init` function which should be called in
///    `rusty_cli_init`.
///
/// 2. a static flag selector variable `SEL` of type
///    `lotto::reexport::Lazy<FlagSel<_>>`.
///
/// The annotated function does not need to be public.
///
/// Accepted arguments:
/// - `name`: **required**
/// - `args`: **required**
/// - `desc`: **required**
/// - `engine_flags`: optional, `false` by default
/// - `cli_flags`: optional, empty by default
/// - `flags`: optional
/// - `group`: optional, `OTHER` by default
///
/// Refer to `subcmd_register` for their meanings.
///
/// # Example
///
/// ```ignore
/// #[lotto::subcmd(name="hello-rust", args="", desc="Example")]
/// fn hello_rust(args: &mut Args, flags: &mut Flags) -> SubCmdResult {
///     println!("Hello Rust");
///     Ok(())
/// }
/// subcmd_init(); // needs to be called on initialization
/// ```
#[proc_macro_attribute]
pub fn subcmd(attrs: TokenStream, item: TokenStream) -> TokenStream {
    cli::subcmd::main(attrs, item)
}

/// Quickly declare a type to be `Marshable` without a print.
#[proc_macro_derive(MarshableNoPrint)]
pub fn marshable_noprint(item: TokenStream) -> TokenStream {
    engine::marshable_noprint::main(item)
}

/// Declare a component (usually a handler) to have states managed by
/// Lotto.
///
/// This macro will automatically generate a `Stateful` implementation
/// for the annotated type, following the conventions.
///
/// A field can be marked as `config`, or `persistent`, or `final`.
/// Each kind can only be attributed to one field.  If, for example,
/// there is no `persistent` field in the struct, it will default to
/// the unit type `()`.
///
/// # Safety
///
/// Beware that the Lotto state manager can modify the states in the
/// background. To avoid potential UB, it's advised to wrap these
/// fields in an interior mutable container.
///
/// # Example
///
/// ```ignore
/// #[derive(Stateful)]
/// struct Handler {
///     #[config]
///     enabled: bool,
///     #[final]
///     tasks: HashMap<TaskId, TaskInfo>,
/// }
/// ```
///
/// This will generate an implementation similar to:
///
/// ```ignore
/// impl Stateful for Handler {
///     type Config = bool;
///     type Final = HashMap<TaskId, TaskInfo>;
///     type Persistent = ();
///     fn config_ptr() -> *mut Self::Config {
///         &self.enabled as *const _ as *mut _
///     }
///     fn final_ptr() -> *mut Self::Final { ... }
///     fn persistent_ptr() -> *mut Self::Persistent {
///         std::ptr::null_ptr()
///     }
/// }
/// ```
#[proc_macro_derive(Stateful, attributes(config, persistent, r#final, result))]
pub fn stateful(input: TokenStream) -> TokenStream {
    brokers::stateful::main(input)
}
