use std::fmt::Write;
use std::path::PathBuf;
use std::process::{Command, Output, Stdio};

use lotto::base::envvar::EnvScope;

use crate::error::Error;

/// A program instruction. Used for result reporting.
#[derive(Clone, Debug)]
pub struct Instruction {
    /// Program path.
    pub path: PathBuf,

    /// Offset of the instruction.
    pub offset: u64,
}

impl Instruction {
    /// Obtain the source line.
    fn display_source_impl(&self) -> Result<String, Error> {
        let output = Command::new("addr2line")
            .arg("-e")
            .arg(&self.path)
            .arg("-f")
            .arg("-C")
            .arg(&format!("{:x}", self.offset))
            .output()?;
        output_to_string(output)
    }

    /// Obtain the assembly code.
    fn display_assembly_impl(&self) -> Result<String, Error> {
        if self.offset == 0 {
            return Ok(format!("NO ASSEMBLY FOR INVALID OFFSET {}", self.offset));
        }
        let objdump = Command::new("objdump")
            .arg("-d")
            .arg("-Mintel")
            .arg(&self.path)
            .stdout(Stdio::piped())
            .spawn()?;
        let grep = Command::new("grep")
            .arg("-A1")
            .arg("-B1")
            .arg(&format!("{:x}", self.offset))
            .stdin(objdump.stdout.unwrap())
            .output()?;
        output_to_string(grep)
    }

    fn with_lotto_disabled(f: impl FnOnce() -> Result<String, Error>) -> Result<String, Error> {
        let _disable = EnvScope::unset("LD_PRELOAD");
        f()
    }

    pub fn display_source(&self) -> Result<String, Error> {
        Self::with_lotto_disabled(|| self.display_source_impl())
    }

    pub fn display_assembly(&self) -> Result<String, Error> {
        Self::with_lotto_disabled(|| self.display_assembly_impl())
    }

    /// Obtain in a mixed, prettified format.
    pub fn display(&self) -> Result<String, Error> {
        Self::with_lotto_disabled(|| {
            let mut res = String::new();
            let assembly = self.display_assembly_impl()?;
            let source = self.display_source_impl()?;
            write!(res, "{}", source).unwrap();
            write!(res, "{}", assembly).unwrap();
            Ok(res)
        })
    }
}

fn output_to_string(output: Output) -> Result<String, Error> {
    if output.status.success() {
        let res = String::from_utf8_lossy(&output.stdout);
        Ok(res.to_string())
    } else {
        let res = String::from_utf8_lossy(&output.stderr);
        Err(Error::SubprocessFailure(res.to_string()))
    }
}
