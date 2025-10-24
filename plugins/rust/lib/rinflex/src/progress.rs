//! # Progress reporting
//!
//! Simple progress bar.

const BAR_WIDTH: usize = 20;

pub struct ProgressBar {
    pub prefix: String,
    pub target: u64,
    pub valid_ticks: u64,
    pub invalid_ticks: u64,
    pub enabled: bool,
    finalized: bool,
}

impl Drop for ProgressBar {
    fn drop(&mut self) {
        if !self.finalized {
            self.done();
        }
    }
}

impl ProgressBar {
    pub fn new(enabled: bool, prefix: &str, target: u64) -> ProgressBar {
        let ret = ProgressBar {
            enabled,
            prefix: prefix.to_string(),
            target,
            valid_ticks: 0,
            invalid_ticks: 0,
            finalized: false,
        };
        ret.refresh();
        ret
    }

    pub fn set_prefix_string(&mut self, prefix: String) -> &mut ProgressBar {
        self.prefix = prefix;
        self
    }

    pub fn reset(&mut self) {
        self.valid_ticks = 0;
        self.invalid_ticks = 0;
    }

    pub fn tick_valid(&mut self) {
        self.valid_ticks += 1;
        self.refresh();
    }

    pub fn tick_invalid(&mut self) {
        self.invalid_ticks += 1;
        self.refresh();
    }

    pub fn refresh(&self) {
        if !self.enabled {
            return;
        }
        let progress_frac = self.valid_ticks as f64 / self.target as f64;
        let progress = (progress_frac * (BAR_WIDTH as f64)) as usize;
        let progress = progress.min(BAR_WIDTH);
        let mut bar = ['░'; BAR_WIDTH];
        for i in 0..progress {
            bar[i] = '█';
        }
        let bar = String::from_iter(&bar);
        eprint!(
            "\r{:9}[{}] {}/{} {}",
            &self.prefix,
            bar,
            self.valid_ticks,
            self.target,
            if self.invalid_ticks > 0 {
                format!(" ({})", self.invalid_ticks)
            } else {
                String::new()
            }
        );
    }

    /// Flush and start a newline.
    pub fn done(&mut self) {
        assert!(!self.finalized);
        self.refresh();
        self.finalized = true;
        eprintln!();
    }
}
