use crate::TrustHandler;
use event_utils::EventType;
use lotto::base::category::Category;
use lotto::engine::handler::AddrSize;
use lotto::engine::handler::Address;
use lotto::engine::handler::AddressValueCatEvent;
use lotto::engine::handler::EventArgs;
use lotto::engine::handler::ExecuteHandler;
use lotto::engine::handler::Reason;
use lotto::engine::handler::ShutdownReason::ReasonShutdown;
use lotto::engine::handler::TaskId;
use lotto::engine::handler::ValuesTypes;
use lotto::engine::handler::{ContextInfo, ExecuteArrivalHandler};
use lotto::log::trace;
use std::fs;
use std::num::NonZeroUsize;
use trust_algorithm::TruST;

#[test]
fn test_read_plus_write() {
    // tests the really simple program
    //         R(x) || W(x)
    // Also checks if the file is correctly being writen and read with
    // the serialized data.
    let file = "trust_test.txt".to_string();
    let addr1 = Address::new(NonZeroUsize::new(4).unwrap());
    let mut handler = TrustHandler::new();
    handler.test_mode = true;
    handler.initialized = true;
    handler.set_file_name(&file);
    match fs::remove_file(file.clone()) {
        Ok(_) => {
            trace!("File {file} deleted.");
        }
        Err(_) => {
            trace!("File {file} not deleted.");
        }
    }
    handler.intialize_content_from_previous_run(); // shouldnt do anything
    let active_tids_1 = vec![TaskId::new(1)];
    let active_tids_1_2 = vec![TaskId::new(1), TaskId::new(2)];
    let active_tids_1_3 = vec![TaskId::new(1), TaskId::new(3)];
    let active_tids_1_2_3 = vec![TaskId::new(1), TaskId::new(2), TaskId::new(3)];

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_execute(TaskId::new(1), ctx); // first execute with no arrival
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(1), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(2), ctx); // execute tid=2 first event

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    handler.handle_execute(TaskId::new(2), ctx);
    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AREAD, Some(addr1));
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    handler.handle_execute(TaskId::new(2), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_FINI, None);
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1); // Last event so it is already removed from tid-set
    handler.handle_execute(TaskId::new(1), ctx); // go back to 1

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(3), ctx); // first event of 3

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_3);
    handler.handle_execute(TaskId::new(3), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AWRITE, Some(addr1));
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_3);
    // assert that we want to restart, due to backwards revisit form the write to the read.
    assert!(result.to_tuple().2.is_some() /* reason is set */);
    handler.handle_execute(TaskId::new(3), ctx);
    // check if the file now has the new context:
    let x = fs::read_to_string(&file).expect("File exists");
    let saved_trust: TruST = TruST::deserialize_and_init(x);
    saved_trust.print_cached_address_info();
    let trace = saved_trust.get_reconstruction_trace();
    assert!(trace[0].1.event_type == EventType::Init);
    assert!(trace[1].0 == TaskId::new(3));
    assert!(trace[1].1.event_type == EventType::Write);
    assert!(trace[2].0 == TaskId::new(2));
    // alright!
    let mut handler = TrustHandler::new();
    handler.test_mode = true;
    handler.initialized = true;
    handler.set_file_name(&file);
    handler.intialize_content_from_previous_run(); // should be able to read from the file
    let saved_trace = handler.trust.get_reconstruction_trace();
    assert!(trace == saved_trace);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_execute(TaskId::new(1), ctx); // first event of thread 1
    let result = handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(1), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    let result = handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(2), ctx); // first event of thread 2

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    let result = handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(1), ctx); // go back to 1, because we want to run task 3 before 2.

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    let result = handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1_2);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(3), ctx); // first event of tid = 3

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_2_3);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(3), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AWRITE, Some(addr1));
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_2_3);
    assert!(result.to_tuple().2 == &Some(Reason::Deterministic));
    handler.handle_execute(TaskId::new(3), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_FINI, None);
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_2);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(2), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AREAD, Some(addr1));
    let result = handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    assert!(result.to_tuple().2 == &Some(Reason::Deterministic));
    handler.handle_execute(TaskId::new(2), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_FINI, None);
    let result = handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(1), ctx);

    // FINISH event:
    let ctx = &get_mocked_context_info(Category::CAT_FUNC_EXIT, None);
    let result = handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    assert!(result.to_tuple().2 == &Some(Reason::Shutdown(ReasonShutdown))); // must rebuild for next execution graph!
    handler.handle_execute(TaskId::new(1), ctx);

    let x = fs::read_to_string(&file).expect("File exists");
    let saved_trust: TruST = TruST::deserialize_and_init(x);
    saved_trust.print_cached_address_info();
    let trace = &saved_trust.get_reconstruction_trace();

    let mut handler = TrustHandler::new();
    handler.initialized = true;
    handler.test_mode = true;
    handler.set_file_name(&file);
    handler.intialize_content_from_previous_run(); // should be able to read from the file
    let saved_trace = &handler.trust.get_reconstruction_trace();
    assert!(saved_trace == trace);

    // simulate. Now the Read comes before the Write and we will end until
    // the very end, where it should report that all graphs were executed.

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_execute(TaskId::new(1), ctx); // first execute with no arrival
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(1), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(2), ctx); // execute tid=2 first event

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    handler.handle_execute(TaskId::new(2), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AREAD, Some(addr1));
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1_2);
    handler.handle_execute(TaskId::new(2), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_FINI, None);
    handler.handle_arrival(TaskId::new(2), ctx, &active_tids_1); // Last event so it is already removed from tid-set
    handler.handle_execute(TaskId::new(1), ctx); // go back to 1

    let ctx = &get_mocked_context_info(Category::CAT_TASK_CREATE, None);
    handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    handler.handle_execute(TaskId::new(3), ctx); // first event of 3

    let ctx = &get_mocked_context_info(Category::CAT_TASK_INIT, None);
    handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_3);
    handler.handle_execute(TaskId::new(3), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_BEFORE_AWRITE, Some(addr1));
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1_3);
    assert!(result.to_tuple().2 == &Some(Reason::Deterministic)); // this time NO backwards revisit.
    handler.handle_execute(TaskId::new(3), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_TASK_FINI, None);
    let result = handler.handle_arrival(TaskId::new(3), ctx, &active_tids_1);
    assert!(result.to_tuple().2.is_none());
    handler.handle_execute(TaskId::new(1), ctx);

    let ctx = &get_mocked_context_info(Category::CAT_FUNC_EXIT, None);
    let _result = handler.handle_arrival(TaskId::new(1), ctx, &active_tids_1);
    // let reason = result.to_tuple().2.unwrap();
    // // We can successfuly finish the program ;)
    // assert!(
    //     reason == lotto::engine::handler::Reason::Shutdown(lotto::engine::handler::ShutdownReason::ReasonSuccess)
    // );
    handler.handle_execute(TaskId::new(1), ctx);
    // Finished then :D
}

use lotto::engine::handler::AddressCatEvent;

fn new_mocked_event_args(cat: Category, addr: Option<Address>) -> EventArgs {
    match cat {
        Category::CAT_BEFORE_AREAD => EventArgs::AddressOnly(
            AddressCatEvent::BeforeARead,
            addr.unwrap(),
            AddrSize::new(4),
        ),
        Category::CAT_BEFORE_WRITE => EventArgs::AddressOnly(
            AddressCatEvent::BeforeWrite,
            addr.unwrap(),
            AddrSize::new(4),
        ),
        Category::CAT_BEFORE_AWRITE => EventArgs::AddressValue(
            AddressValueCatEvent::BeforeAWrite,
            addr.unwrap(),
            AddrSize::new(4),
            ValuesTypes::U32(1),
        ),
        _ => EventArgs::NoChanges,
    }
}

/// Useful for testing purposes only!
fn new_mocked_context(cat: Category, pc: usize, addr1: Option<Address>) -> ContextInfo {
    ContextInfo {
        event_args: new_mocked_event_args(cat, addr1),
        pc,
        cat,
    }
}

fn get_mocked_context_info(cat: Category, addr: Option<Address>) -> ContextInfo {
    new_mocked_context(cat, 1, addr)
}
