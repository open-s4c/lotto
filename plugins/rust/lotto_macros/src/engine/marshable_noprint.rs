use proc_macro::TokenStream;
use quote::quote;
use syn::parse_macro_input;
use syn::ItemStruct;

pub(crate) fn main(item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as ItemStruct);
    let ident = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();
    let expanded = quote! {
        impl #impl_generics ::lotto::brokers::statemgr::Marshable for #ident #ty_generics #where_clause {}
    };
    TokenStream::from(expanded)
}
