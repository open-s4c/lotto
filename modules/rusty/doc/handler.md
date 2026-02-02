There are two traits your handler could implement from, or from both.

# Review on categories of events

Lotto intercepts the client at points of the execution that belong to some category.
For example, the category `CAT_BEFORE_AWRITE` means that the current thread will 
execute an 
atomic write in it's `next` operation. The category `CAT_AFTER_AWRITE` means that 
it *just* 
wrote. 

It is worth noting that when a handler gets from lotto an event on the form `cat = 
CAT_BEFORE_AWRITE, tid = x`,
this means that we have the **intention** of running thread `x` 
and that it has been intercepted at a point with category `CAT_BEFORE_AWRITE`. But 
the 
switcher **can change** the running thread. Therefore, we would only be able to 
detect that 
the write actually occurred in the **next** interception of `x`. This may already 
be to late for the purposes of the handler, for example, if we need to know the 
value of the variable to be written *right before* it is modified by `x`. 
Luckly, you can use the following trait to circumvent this problem.

> Note: some categories may be dynamically set, for example, `AFTER_CMPXCHG_F` and 
`AFTER_CMPXCHG_S`. Most of them are hard-coded.

# ExecuteHandler trait

This is meant for all the handlers that need to know that an event is happening 
just when the operation is **really** executed. This is possible due to a callback 
from the 
`sequencer_resume`, which happens after the final scheduling decision is made but 
before running, so it is possible to get the current values for each address.
This type of handler can be thought as an **observer**.

Note that in this way we can only make decision of blocking/unblocking tasks for 
the **next** event.

## Methods

`fn handle_execute(&mut self, tid: u64, ctx_info: &ContextInfo);`: Add here all the
logic of the handler. Note that it has no return value.


# ExecuteArrivalHandler trait

These handlers additionally receive callbacks **before** the final scheduling 
decision is made.
This is useful for handlers that have to block/unblock tasks as soon as the 
interception happens. 

## Methods

`fn handle_arrival(&mut self, tid: u64, ctx_info: &ContextInfo) -> EventResult;`:
must return a list of blocked_tasks and a boolean indicating whether it is a change 
point.

> Note that if the event is marked as read only, the handler will not be able
> to block any task. This happens, for example, if the watchdog is triggered


# Subscribing your handler

Suppose we have a handler called `struct MyHandler`. To register it, create a 
function of the following template and call it from the [rusty engine](../
rusty_engine/src/lib.rs) by adding it into the function `lotto_rust_handlers_init`

```rs
fn register(){
    let my_handler = my_handler::MyHandler::new();
    handler_rusty_pubsub::subscribe_execute(Box::new(my_handler));
    // ^ subscribe to the desired mode.
}
```
