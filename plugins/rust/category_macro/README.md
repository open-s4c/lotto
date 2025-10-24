# CategoryMacro 

## How to initialise Cargo.toml

Add the following to the [dependencies] in the Cargo.toml of the handler: 

```rust
as-any = "0.3.1"
category_macro = { path = "../../../rust/category_macro" }
syn = { version = "1.0.60", features = ["extra-traits", "full", "fold"] }
quote = "1.0.9"
proc-macro2 = "1.0.24"
proc-macro-error = "1.0"
```


## How to initialise the proc-macro in the handler file

In the handler file: 

```rust
use category_macro::CategoryMacro
use lotto::engine::pubsub::CustomCatTrait
use as_any::Downcast
```

Contextfields the proc-macro can generate out of the C context: 

```rust
#[derive(CategoryMacro)]
#[handler_type(HANDLER)]
pub struct Custom {
    name: *const i8,
    tid: TaskId,
    address: Address,
    size: AddrSize,
    val: ValuesTypes,
}
```

(`HANDLER` should be the handler that uses the `Custom` Category.)


## How it works and examples

- proc macro generates a function for parsing a C context_t into a `Custom` struct
- the function automatically gets invoked when a C context_t with this event's custom category number is intercepted

### Generation of parser
- proc macro looks at each field of the `Custom` struct and generates code that extracts the corresponding data from the C context_t object
- case 1: data is stored in fields of the context_t (e.g., Thread ID of the intercepted thread and the name string)
- case 2: remaining data is stored in the values-array of the context_t

To distinguish between these two cases, the proc macro uses the following rules:
- if the name of the field is `name` or `tid`: data is taken from the corresponding field of context_t
- we refer to all other fields as value fields 
- for the n-th value field of `Custom`: data is taken from the n-th element of the values-array

Since the latter involves some conversion between a C type and a Rust type, only the following types of value fields are supported: `Address`, `AddrSize`, `ValuesType`.

Other valid examples: 
```rust
#[derive(CategoryMacro)]
#[handler_type(HANDLER)]
pub struct CustomExample1 {
    val1: ValuesTypes,
    size: AddrSize,
    val2: ValuesTypes
    name: *const i8,
}

#[derive(CategoryMacro)]
#[handler_type(HANDLER)]
pub struct CustomExample2 {
    address: Address,
}

#[derive(CategoryMacro)]
#[handler_type(HANDLER)]
pub struct CustomExample3 {
    val1: ValuesTypes,
    val2: ValuesTypes,
    val3: ValuesTypes,
}

#[derive(CategoryMacro)]
#[handler_type(HANDLER)]
pub struct CustomExample4 {
    size: AddrSize,
    tid: TaskId,
}
```

## What is can and cannot generate

### Can generate

- CustomCatTrait implementation for `Custom`
- `Custom`ContextHandler trait creation, that implements fn handle_`custom`_context
- Category for `Custom`: CAT_`CUSTOM`, can be accessed with `custom`_cat()
- fn parse_`custom`_context stores the parser, that can parse a context of type context_t into the `Custom` struct
- fn parse implementation for `Custom` that returns a `Custom` struct
- `CUSTOM` and `custom` are in snake case convention: `custom_example`

### Cannot generate / needs to be added manually
- implementation of `Custom`ContextHandler for `HANDLER` with fn handle_`custom`_context(&mut self, custom_context: &`Custom`): what the handler actually does when it's a custom category 
- implementation of other custom categories trait with its handle function (if needed)
- registeration of custom category (see next Section)


## How to register custom categories with its context parsers

(please replace the custom with Your `Custom` in lower case)

In pub fn register():

```rust
{
    let _ = custom_cat();
    let custom_cat_num = custom_cat().0;
    let table = &mut lotto::engine::handler::ENGINE_DATA
                .try_lock()
                .expect("single threaded")
                .custom_cat_table;
    table.insert(custom_cat_num, parse_custom_context());
}
```

## Usage of custom categories from Rust in C (intercept)

```c
category_t custom_cat();
```
