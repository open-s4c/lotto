use std::num::NonZeroI32;

use lotto::base::Clock;
use lotto::cli::HasErrorCode;
use std::path::PathBuf;
use thiserror::Error;

use std::io;

#[derive(Error, Debug)]
pub enum Error {
    #[error("Interrupted by user")]
    Interrupted,

    #[error("No records with the specified clock {} found in trace {}", .1, .0.display())]
    ClockNotFound(PathBuf, Clock),

    #[error("No event found at the specified clk {} in trace {}", .1, .0.display())]
    EventNotFound(PathBuf, Clock),

    #[error("The execution terminated due to lotto internal error")]
    LottoError,

    #[error("System IO error: {}", .0)]
    IO(#[from] io::Error),

    #[error("Subprocess exited abnormally with stderr {}", .0)]
    SubprocessFailure(String),

    #[error("Cannot find an execution that satisfies all of the given constraints")]
    ExecutionNotFound,
}

impl HasErrorCode for Error {
    fn error_code(&self) -> NonZeroI32 {
        match self {
            Error::Interrupted => NonZeroI32::new(130).unwrap(),
            Error::ClockNotFound(_, _) => NonZeroI32::new(1).unwrap(),
            Error::EventNotFound(_, _) => NonZeroI32::new(2).unwrap(),
            Error::LottoError => NonZeroI32::new(3).unwrap(),
            Error::IO(_) => NonZeroI32::new(4).unwrap(),
            Error::SubprocessFailure(_) => NonZeroI32::new(5).unwrap(),
            Error::ExecutionNotFound => NonZeroI32::new(6).unwrap(),
        }
    }
}
