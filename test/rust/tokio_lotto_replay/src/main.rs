use std::sync::{
    atomic::{AtomicBool, AtomicUsize, Ordering},
    Arc,
};

struct Shared {
    ready: AtomicBool,
    value: AtomicUsize,
}

#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() {
    for attempt in 0..64 {
        let shared = Arc::new(Shared {
            ready: AtomicBool::new(false),
            value: AtomicUsize::new(0),
        });

        let writer_shared = Arc::clone(&shared);
        let writer = tokio::task::spawn_blocking(move || {
            std::thread::yield_now();

            // Bug: publish readiness before publishing the data.
            writer_shared.ready.store(true, Ordering::Release);
            for _ in 0..32 {
                std::thread::yield_now();
            }
            writer_shared.value.store(1, Ordering::Release);
        });

        let reader_shared = Arc::clone(&shared);
        let reader = tokio::task::spawn_blocking(move || {
            while !reader_shared.ready.load(Ordering::Acquire) {
                std::thread::yield_now();
            }

            let observed = reader_shared.value.load(Ordering::Acquire);
            assert_eq!(
                observed, 1,
                "tokio replay bug: observed stale value {observed} on attempt {attempt}"
            );
        });

        let (writer, reader) = tokio::join!(writer, reader);
        writer.expect("writer task failed");
        reader.expect("reader task failed");
    }
}
