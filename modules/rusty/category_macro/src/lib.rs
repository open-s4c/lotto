extern crate proc_macro;

use convert_case::{Case, Casing};
use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, Attribute, DeriveInput, Field};

#[proc_macro_derive(CategoryMacro, attributes(handler_type))]
pub fn category_derive(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);

    let struct_name = &input.ident;

    let fields = if let syn::Data::Struct(s) = &input.data {
        s.fields.iter().collect::<Vec<&Field>>()
    } else {
        panic!("CategoryMacro can only be derived on structs");
    };

    let parse_method = generate_parse_method(struct_name, &fields);

    let parse = quote! {
        impl #struct_name {
            fn parse(context: &lotto::raw::context_t) -> Self {
                let mut counter = 0;
                #parse_method
            }
        }
    };

    let context_name = struct_name.to_string();
    let context_ident = syn::Ident::new(&context_name, struct_name.span());

    let struct_name_str = context_name.to_case(Case::Snake);
    let parser_name = format!("parse_{}_context", struct_name_str);
    let parser_ident = syn::Ident::new(&parser_name, struct_name.span());

    let parse_function = quote! {
        fn #parser_ident() -> Box<dyn Fn(&lotto::raw::context_t) -> Box<dyn CustomCatTrait> + Send + Sync> {
            Box::new(|context: &lotto::raw::context_t| {
            Box::new(#struct_name::parse(context)) as Box<dyn CustomCatTrait>
        })
        }
    };

    let function_name = format!("handle_{}_context", struct_name_str);
    let function_ident = syn::Ident::new(&function_name, struct_name.span());

    let trait_name = format!("{}ContextHandler", struct_name);
    let trait_ident = syn::Ident::new(&trait_name, struct_name.span());

    let trait_impl = quote! {
        pub trait #trait_ident {
            fn #function_ident(&mut self, context: &#context_ident);
        }
    };

    let handler_type = input
        .attrs
        .iter()
        .find_map(|attr: &Attribute| {
            if attr.path().is_ident("handler_type") {
                attr.parse_args::<syn::Path>().ok()
            } else {
                None
            }
        })
        .expect("Missing or invalid handler_type attribute");

    let custom_cat_trait_implementation = quote! {
        impl CustomCatTrait for #struct_name {
            fn call_right_handler(&self, handler: &mut (dyn lotto::engine::handler::ArrivalOrExecuteHandler + Send + Sync)) {
                if let Some(h) = handler.downcast_mut::<#handler_type>() {
                    h.#function_ident(self);
                }
            }
        }
    };

    let cat_name_str = struct_name_str.to_uppercase();
    let cat_name = format!("CAT_{}", cat_name_str);
    let cat_ident = syn::Ident::new(&cat_name, struct_name.span());

    let cat_fn_name = format!("{}_cat", struct_name_str);
    let cat_fn_ident = syn::Ident::new(&cat_fn_name, struct_name.span());

    let cat_fn = quote! {
        #[no_mangle]
        pub extern "C" fn #cat_fn_ident() -> lotto::base::category::Category {
            static #cat_ident: lotto::brokers::catmgr::CategoryKey = lotto::brokers::catmgr::CategoryKey::new(unsafe {
                std::ffi::CStr::from_bytes_with_nul_unchecked(concat!(#cat_name, "\0").as_bytes())
            });
            #cat_ident.get()
        }
    };

    let implementation = quote! {
        #custom_cat_trait_implementation

        #trait_impl

        #cat_fn

        #parse

        #parse_function
    };

    TokenStream::from(implementation)
}

fn generate_parse_method(
    struct_name: &syn::Ident,
    fields: &[&syn::Field],
) -> proc_macro2::TokenStream {
    let mut field_assignments = Vec::new();

    for field in fields {
        let field_name = &field.ident;
        let field_type = &field.ty;

        if let syn::Type::Path(type_path) = field_type {
            let type_name = type_path.path.segments.last().unwrap().ident.to_string();

            match type_name.as_str() {
                "Address" => {
                    field_assignments.push(quote! {
                        #field_name: {
                    let address = if let Ok(addr) = lotto::engine::handler::get_addr_from_context(context, counter) {
                        lotto::engine::handler::Address::new(addr)
                    } else {
                        trace!("The DUT called an interceptor passing a nullptr as payload field address for an event of type Address. Event Address only accepts non-null addresses in field address");
                        assert!(false);
                        lotto::engine::handler::Address::new(std::num::NonZeroUsize::new(1).expect("Setting default address failed"))
                    };
                            let ad = address;
                            counter+=1;
                        ad},
                    });
                }
                "TaskId" => {
                    field_assignments.push(quote! {
                        #field_name: lotto::base::TaskId::new(context.id),
                    });
                }
                "ValuesTypes" => {
                    field_assignments.push(quote! {
                        #field_name: {
                            let value = lotto::engine::handler::get_value_from_context(context,counter,lotto::engine::handler::AddrSize::new(4));
                            counter+=1;
                            value },
                    });
                }
                "AddrSize" => {
                    field_assignments.push(quote! {
                        #field_name: {
                            let size = lotto::engine::handler::get_size_from_context(context,counter);
                            counter+=1;
                            size },
                    });
                }
                _ => {
                    panic!("No such type in Context!");
                }
            }
        } else if let syn::Type::Ptr(ptr) = field_type {
            if let syn::Type::Path(type_path) = &*ptr.elem {
                let type_name = type_path.path.segments.last().unwrap().ident.to_string();
                if type_name == "i8" {
                    field_assignments.push(quote! {
                        #field_name: context.func,
                    });
                }
            }
        }
    }

    // Generate the final struct creation code
    quote! {
        #struct_name {
            #(#field_assignments)*
        }
    }
}
