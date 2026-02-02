use darling::ast::NestedMeta;
use darling::FromMeta;
use proc_macro::TokenStream;
use quote::quote;
use syn::parse::{Parse, ParseStream};
use syn::parse_macro_input;
use syn::punctuated::Punctuated;
use syn::{Expr, ItemFn, LitStr, Result, Token};

#[derive(Debug, FromMeta)]
struct Attrs {
    name: LitStr,
    args: LitStr,
    desc: LitStr,
    engine_flags: Option<Expr>,
    cli_flags: Option<Expr>,
    defaults: Option<Expr>,
    group: Option<Expr>,
}

pub(crate) fn main(attrs: TokenStream, item: TokenStream) -> TokenStream {
    let attrs = parse_macro_input!(attrs as Attrs);
    let input_fn = parse_macro_input!(item as ItemFn);
    let input_fn_ident = &input_fn.sig.ident;

    let name = cstr(&attrs.name);
    let args = cstr(&attrs.args);
    let desc = cstr(&attrs.desc);
    let engine_flags = match attrs.engine_flags {
        None => quote! { false },
        Some(exp) => quote! { #exp },
    };
    let cli_flags = match attrs.cli_flags {
        None => quote! { [] },
        Some(exp) => quote! { #exp },
    };
    let (defaults, defaults_def) = match attrs.defaults {
        None => (quote! { Some(raw::flags_default) }, quote! {}),
        Some(exp) => (
            quote! { Some(_default_flags) },
            quote! {
                unsafe extern "C" fn _default_flags() -> *mut raw::flags_t {
                    let mut flags = Flags::default().to_owned();
                    (#exp)(&mut flags);
                    flags.into_raw()
                }
            },
        ),
    };
    let group = match attrs.group {
        None => quote! { raw::subcmd_group::SUBCMD_GROUP_OTHER },
        Some(exp) => quote! { #exp },
    };

    let expanded = quote! {
        #input_fn

        pub(crate) fn subcmd_init() {
            use ::lotto::raw;
            use ::lotto::base::Flags;
            use ::lotto::cli::Args;
            use ::core::ptr;

            #defaults_def

            extern "C" fn trampoline(args: *mut raw::args_t, mut flags: *mut raw::flags_t) -> i32 {
                unsafe {
                    let args = &mut *(args as *mut Args);
                    let flags = &mut *(flags as *mut Flags);
                    let status: lotto::cli::SubCmdResult = #input_fn_ident(args, flags);
                    match status {
                        Ok(_) => 0,
                        Err(e) => e.0.into(),
                    }
                }
            }

            let mut sel = vec! #cli_flags;
            sel.push(0);

            unsafe {
                raw::subcmd_register(
                    Some(trampoline),
                    #name,
                    #args,
                    #desc,
                    #engine_flags,
                    sel.as_ptr(),
                    #defaults,
                    #group,
                );
            }
        }
    };
    TokenStream::from(expanded)
}

fn cstr(lit: &LitStr) -> proc_macro2::TokenStream {
    quote! {
        unsafe {
            ::std::ffi::CStr::from_bytes_with_nul_unchecked(concat!(#lit, "\0").as_bytes()).as_ptr()
        }
    }
}

impl Parse for Attrs {
    fn parse(input: ParseStream) -> Result<Self> {
        let items: Vec<_> = Punctuated::<NestedMeta, Token![,]>::parse_terminated(input)?
            .into_iter()
            .collect();
        Ok(Self::from_list(&items)?)
    }
}
