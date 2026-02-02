use proc_macro::TokenStream;
use quote::{quote, quote_spanned};
use syn::{
    parse_macro_input, spanned::Spanned, token, Data, DeriveInput, Field, Fields, Type, TypeTuple,
};

pub fn main(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = &input.ident;

    let mut config_field = None;
    let mut persistent_field = None;
    let mut final_field = None;

    if let Data::Struct(data_struct) = &input.data {
        if let Fields::Named(fields) = &data_struct.fields {
            for field in &fields.named {
                for attr in &field.attrs {
                    match attr.path().get_ident().map(|i| i.to_string()) {
                        Some(s) if s == "config" => {
                            if config_field.is_some() {
                                return quote_spanned! { field.span() =>
                                    compile_error!("Multiple `config` fields are not allowed");
                                }
                                .into();
                            }
                            config_field = Some(field)
                        }
                        Some(s) if s == "persistent" => {
                            if persistent_field.is_some() {
                                return quote_spanned! { field.span() =>
                                    compile_error!("Multiple `persistent` fields are not allowed");
                                }
                                .into();
                            }
                            persistent_field = Some(field)
                        }
                        Some(s) if s == "final" || s == "result" => {
                            if final_field.is_some() {
                                return quote_spanned! { field.span() =>
                                    compile_error!("Multiple `final` or `result` fields are not allowed");
                                }.into();
                            }
                            final_field = Some(field)
                        }
                        _ => {}
                    }
                }
            }
        }
    }

    let unit_type = Type::Tuple(TypeTuple {
        paren_token: token::Paren::default(),
        elems: Default::default(),
    });
    let config_type = config_field.map(|f| &f.ty).unwrap_or(&unit_type);
    let persistent_type = persistent_field.map(|f| &f.ty).unwrap_or(&unit_type);
    let final_type = final_field.map(|f| &f.ty).unwrap_or(&unit_type);

    let config_ptr_expr = get_ptr_expr(config_field);
    let persistent_ptr_expr = get_ptr_expr(persistent_field);
    let final_ptr_expr = get_ptr_expr(final_field);

    let expanded = quote! {
        impl ::lotto::brokers::statemgr::Stateful for #name {
            type Config = #config_type;
            type Persistent = #persistent_type;
            type Final = #final_type;

            fn config_ptr(&self) -> *mut Self::Config {
                #config_ptr_expr
            }

            fn persistent_ptr(&self) -> *mut Self::Persistent {
                #persistent_ptr_expr
            }

            fn final_ptr(&self) -> *mut Self::Final {
                #final_ptr_expr
            }
        }
    };

    TokenStream::from(expanded)
}

fn get_ptr_expr(field: Option<&Field>) -> proc_macro2::TokenStream {
    if let Some(field) = field {
        let fname = &field.ident;
        quote! { &self.#fname as *const _ as *mut _ }
    } else {
        quote! { std::ptr::null_mut() }
    }
}
