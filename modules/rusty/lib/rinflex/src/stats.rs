use std::time::Duration;
use std::{
    sync::{
        atomic::{AtomicU64, Ordering},
        LazyLock, Mutex,
    },
    time::Instant,
};

#[derive(Default, Debug)]
pub struct Stats {
    num_runs: AtomicU64,
    num_discarded_runs: AtomicU64,
    duration: Mutex<Duration>,
}

pub static STATS: LazyLock<Stats> = LazyLock::new(|| Default::default());

impl Stats {
    pub fn tick_run(&self) -> u64 {
        self.num_runs.fetch_add(1, Ordering::Relaxed)
    }

    pub fn tick_discarded_run(&self) -> u64 {
        self.num_discarded_runs.fetch_add(1, Ordering::Relaxed)
    }

    pub fn count_run_time(&self, start: Instant) -> Duration {
        let mut duration = self.duration.try_lock().expect("single threaded");
        *duration += start.elapsed();
        *duration
    }

    pub fn report(&self) {
        let duration = self.duration.try_lock().expect("single threaded");
        println!(
            "#Executions:           {}",
            self.num_runs.load(Ordering::Relaxed)
        );
        println!(
            "#Discarded executions: {}",
            self.num_discarded_runs.load(Ordering::Relaxed)
        );
        println!("Total time inside SUT: {} ms", duration.as_millis());
    }
}
