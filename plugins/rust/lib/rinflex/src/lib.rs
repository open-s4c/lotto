//! # RInflex
//!
//! This module defines the essential data structures that are shared
//! by both the rinflex CLI and the engine handlers.

pub mod error;
pub mod handlers;
pub mod idmap;
pub mod inflex;
pub mod modify;
pub mod progress;
pub mod rec_inflex;
pub mod report;
pub mod trace;
pub mod vaddr;

use lotto::base::{Category, Clock, StableAddress, StableAddressMethod, TaskId};
use lotto::brokers::{Decode, Encode};
use lotto::raw;
use modify::Modify;
use report::Instruction;
use std::hash::Hash;
use std::path::PathBuf;

/// A transition of a program state.
///
/// A transition is only defined by the essentials of an event --
/// something happened, and what.  It doesn't concern itself with any
/// history or program state.
#[derive(Encode, Decode, Clone, Debug, PartialEq, Eq, Hash)]
pub struct Transition {
    pub id: TaskId,
    pub pc: StableAddress,
    pub cat: Category,
}

impl Transition {
    /// Create a new [`Transition`] from a Lotto capture.
    pub fn new(ctx: &raw::context) -> Self {
        Transition {
            id: TaskId::new(ctx.id),
            cat: ctx.cat,
            pc: StableAddress::with_method(ctx.pc, StableAddressMethod::STABLE_ADDRESS_METHOD_MAP),
        }
    }
}

impl std::fmt::Display for Transition {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            r"{{
  id: {},
  cat: {},
  pc: {},
}}",
            self.id, self.cat, self.pc
        )
    }
}

/// A unique identifier for a stackframe.
///
/// Used in stacktrace.
#[derive(Debug, Clone, Eq, PartialEq, Encode, Decode, Hash)]
pub struct StackFrameId {
    /// Symbol name
    pub sname: Option<String>,

    /// File name
    pub fname: String,

    /// The PC of the CALL instruction (inside caller).
    pub call_pc: StableAddress,

    /// Return address in the executable (inside caller).
    pub offset: usize,
}

impl std::fmt::Display for StackFrameId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if let Some(sname) = &self.sname {
            return write!(f, "{}", sname);
        }
        write!(f, "{}(+{:04x})", self.fname, self.offset)
    }
}

/// The stacktrace when an event happened.
pub type StackTrace = Vec<StackFrameId>;

/// A concrete event.
///
/// It is basically a transition with some metadata.
///
/// An event happens only if the corresponding transition is enabled
/// for the program state. Therefore, an event is essentially a tuple
/// of `(ProgramState, Transition)`, and it results in a new program
/// state.
#[derive(Encode, Decode, Clone, Debug, Hash)]
pub struct Event {
    /// The program transition to which this event is associated.
    pub t: Transition,

    /// The clock when this event actually happened ('resumed').
    pub clk: Clock,

    /// The number of times this transition happened.
    pub cnt: u64,

    /// Stacktrace.
    pub stacktrace: StackTrace,

    /// Modify information.
    pub m: Option<Modify>,
}

impl std::fmt::Display for Event {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{} [cnt={}, clk={}]",
            self.t,
            self.cnt,
            self.clk //self.t, self.clk, self.cnt, self.m, self.stacktrace
        )
    }
}

impl Event {
    /// Compares the equality of two events.
    ///
    /// NOTE: This corresponds to in `event_equals_light` the C code,
    /// whereas the default `PartialEq` corresponds to `event_equals.`
    pub fn equals(&self, other: &Self) -> bool {
        self.t == other.t && self.stacktrace == other.stacktrace
    }

    /// Create an [`Event`] from a Lotto capture that only fills in
    /// `t` and `clk`.
    ///
    /// Peripheral information is not provided.
    pub fn new(ctx: &raw::context, e: &raw::event_t, stacktrace: StackTrace) -> Self {
        Self {
            t: Transition::new(ctx),
            clk: e.clk,
            cnt: 0,
            stacktrace,
            m: None,
        }
    }

    pub fn instruction(&self) -> report::Instruction {
        let ma = self.t.pc.as_map_address();
        let name = ma.name().to_str().expect("invalid utf-8");
        let offset = ma.offset();
        let path = PathBuf::from(name);
        report::Instruction { path, offset }
    }

    /// Display this event.
    pub fn display(&self) -> Result<String, error::Error> {
        let mut result = format!(
            "event - tid: {}, clk: {}, {} x pc: {}, cat: {}\n[{}]\n{}",
            self.t.id,
            self.clk,
            self.cnt,
            self.t.pc,
            self.t.cat,
            self.stacktrace
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<_>>()
                .join(","),
            self.instruction().display()?
        );
        let n = self.stacktrace.len();
        if n < 2 {
            return Ok(result);
        }
        let mut j = n - 1;
        while j >= 1 {
            let callee = &self.stacktrace[j];
            let caller = &self.stacktrace[j - 1];
            let call_pc_map_addr = callee.call_pc.as_map_address();
            let call_insn = Instruction {
                path: PathBuf::from(&caller.fname),
                offset: call_pc_map_addr.offset,
            };
            if let Ok(line) = call_insn.display_source() {
                result.push_str(&format!(
                    "--> [{}] Called by {}\n{}",
                    j,
                    line.replace("\n", " "),
                    call_insn.display_assembly().unwrap_or("".to_string()),
                ));
            } else {
                result.push_str(&format!("--> [{}] Called by {}\n", j, caller));
                if let Ok(asm) = call_insn.display_assembly() {
                    result.push_str(&asm);
                }
            }
            j -= 1;
        }
        Ok(result)
    }
}

/// [`PrimitiveConstraint`] denotes a simple, positive ordering
/// constraint.
///
/// `order_enforcer` will modify ONLY counters.
#[derive(Encode, Decode, Clone, Debug, Hash)]
pub struct PrimitiveConstraint {
    pub source: Event,
    pub target: Event,
}

impl std::fmt::Display for PrimitiveConstraint {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "({}) ==> ({})", self.source, self.target)
    }
}

impl PrimitiveConstraint {
    /// Create a new constraint with flipped source and target.
    pub fn flipped(&self) -> Self {
        let mut flipped = self.clone();
        std::mem::swap(&mut flipped.source, &mut flipped.target);
        flipped
    }

    pub fn display(&self) -> Result<String, error::Error> {
        let res = format!("{}\n{}", self.source.display()?, self.target.display()?,);
        Ok(res)
    }

    pub fn equals(&self, other: &Self) -> bool {
        self.source.equals(&other.source) && self.target.equals(&other.target)
    }

    pub fn clock_lower_bound(&self) -> Clock {
        self.source.clk.min(self.target.clk)
    }
}

/// [`Constraint`] adds some metadata to [`PrimitiveConstraint`].
#[derive(Encode, Decode, Clone, Debug, Hash)]
pub struct Constraint {
    /// The primitive constraint that will be enforced.
    pub c: PrimitiveConstraint,

    /// Whether this constraint is virtual.
    ///
    /// A virtual constraint is not really part of a bug.  It is only
    /// used for performance.
    pub virt: bool,

    /// Marks whether this constraint is originally a positive
    /// constraint.  This does not affect whether `c` must be
    /// satisfied or dissatisfied.
    ///
    /// The constraint `A => B` is represented as
    /// `{A=>B, positive:true}`.
    ///
    /// The constraint, !(A => B), is represented as
    /// `{ B=>A, positive:false }`.
    pub positive: bool,
}

impl std::fmt::Display for Constraint {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.virt {
            write!(f, "[VIRT] ")?;
        }
        if self.positive {
            write!(f, "{}", self.c)?;
        } else {
            write!(f, "!!({}))", self.c)?;
        }
        Ok(())
    }
}

/// A set of constraints.
///
/// NOTE: Corresponds to `ocbag_t` in the C code.
pub type ConstraintSet = Vec<Constraint>;
