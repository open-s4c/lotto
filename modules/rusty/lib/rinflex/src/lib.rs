//! # RInflex
//!
//! This module defines the essential data structures that are shared
//! by both the rinflex CLI and the engine handlers.

pub mod error;
pub mod handlers;
pub mod idmap;
pub mod inflex;
pub mod memory_access;
pub mod num;
pub mod progress;
pub mod rec_inflex;
pub mod report;
pub mod stats;
pub mod trace;
pub mod vaddr;

use lotto::base::{Category, Clock, StableAddress, StableAddressMethod, TaskId};
use lotto::brokers::{Decode, Encode};
use lotto::log::*;
use lotto::raw;
use memory_access::{MemoryAccess, Modify, ModifyKind};
use report::Instruction;
use std::hash::Hash;
use std::marker::PhantomData;
use std::path::PathBuf;
use std::sync::atomic::{fence, Ordering};

/// A transition of a program state.
///
/// A transition is a state transition out of context. It doesn't
/// concern itself with any history or program state. It is almost
/// meaningless without proper context ([`Event`]).
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
        write!(f, r"[id:{}, cat:{}, pc:{}]", self.id, self.cat, self.pc)
    }
}

#[derive(Debug, Clone, Eq, PartialEq, Encode, Decode, Hash)]
pub enum Either<L, R> {
    Left(L),
    Right(R),
}

/// A unique identifier for a stackframe.
///
/// Used in stacktrace.
#[derive(Debug, Clone, Eq, PartialEq, Encode, Decode, Hash)]
pub struct StackFrameId {
    /// Symbol name, or offset of the call instruction
    pub sname: Either<String, u64>,

    /// File name
    pub fname: String,

    /// The PC of the CALL instruction (inside caller).
    pub caller_pc: StableAddress,
}

impl std::fmt::Display for StackFrameId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if let Either::Left(sname) = &self.sname {
            return write!(f, "{}", sname);
        }
        let fname = self.fname.rsplit("/").next().unwrap_or(&self.fname);
        write!(
            f,
            "{}(+{:04x})",
            fname,
            self.caller_pc.as_map_address().offset
        )
    }
}

/// The stacktrace when an event happened.
#[derive(Clone, Encode, Decode, Debug, Hash, PartialEq, Eq, Default)]
pub struct StackTrace(pub Vec<StackFrameId>);

impl std::fmt::Display for StackTrace {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (i, stid) in self.0.iter().enumerate() {
            write!(f, "{}{}", if i == 0 { "[" } else { ", " }, stid)?;
        }
        write!(f, "]")?;
        Ok(())
    }
}

/// A concrete event.
///
/// It is essentially a transition enriched with important auxiliary
/// data.
///
/// An event happens only if the corresponding transition is enabled
/// for the program state. Therefore, an event can be viewed as a
/// tuple of `(ProgramState, Transition)`, and it results in a new
/// program state.
///
/// For more information regarding why the definition includes Modify
/// and Read, refer to the documentation of the [memory_access] mod.
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

    /// Memory access information.
    pub m: Option<MemoryAccess>,
}

/// The effect of a memory operation.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub enum Eff {
    Trivial,
    XchgSame,
    XchgChanged,
    RmwSame,
    RmwChanged,
    CasSuccess,
    CasFailure,
}

#[inline]
fn eval_rmw(op: raw::rmw_op::Type, old: u64, new: u64) -> u64 {
    use raw::rmw_op::*;
    match op {
        RMW_OP_ADD => old.wrapping_add(new),
        RMW_OP_SUB => old.wrapping_sub(new),
        RMW_OP_OR => old | new,
        RMW_OP_AND => old & new,
        RMW_OP_XOR => old ^ new,
        RMW_OP_NAND => !(old & new),
        _ => unreachable!("unknown rmw op {:?}", op),
    }
}

impl From<&MemoryAccess> for Eff {
    fn from(value: &MemoryAccess) -> Self {
        use std::cmp::Ordering::*;
        match value.to_modify() {
            Some(Modify {
                read_value: Some(old),
                kind: ModifyKind::Xchg { value: new, .. },
                ..
            }) => match new.cmp(old) {
                Equal => Eff::XchgSame,
                _ => Eff::XchgChanged,
            },
            Some(Modify {
                read_value: Some(old),
                kind:
                    ModifyKind::Rmw {
                        delta,
                        operator: op,
                        ..
                    },
                ..
            }) => {
                let new = eval_rmw(*op, *old, *delta);
                match new.cmp(old) {
                    Equal => Eff::RmwSame,
                    _ => Eff::RmwChanged,
                }
            }
            Some(Modify {
                read_value: Some(old),
                kind: ModifyKind::Cas { expected, .. },
                ..
            }) => {
                if old == expected {
                    Eff::CasSuccess
                } else {
                    Eff::CasFailure
                }
            }
            _ => Eff::Trivial,
        }
    }
}

impl From<Option<&MemoryAccess>> for Eff {
    fn from(value: Option<&MemoryAccess>) -> Self {
        if let Some(ma) = value {
            Eff::from(ma)
        } else {
            Eff::Trivial
        }
    }
}

/// The equivalence "core" of an [`Event`].
///
/// `c1 == c2` if and only if `e1.equals(e2)`
#[derive(PartialEq, Eq, Clone, Debug, Hash)]
pub struct GenericEventCore<StackTraceType> {
    pub t: Transition,
    pub stacktrace: StackTraceType,
    // pub addr: Option<vaddr::VAddr>,
    pub eff: Eff,
    pub _phantom: PhantomData<StackTraceType>,
}

#[derive(PartialEq, Eq, Clone, Debug, Hash)]
pub struct GenericEventCoreRef<'a, StackTraceType> {
    pub t: &'a Transition,
    pub stacktrace: &'a StackTraceType,
    // pub addr: Option<&'a vaddr::VAddr>,
    pub eff: Eff,
    pub _phantom: PhantomData<&'a StackTraceType>,
}

type EventCore = GenericEventCore<StackTrace>;
type EventCoreRef<'a> = GenericEventCoreRef<'a, StackTrace>;

impl<S1> GenericEventCore<S1> {
    pub fn from_ref<'a, S2>(
        r: GenericEventCoreRef<'a, S1>,
        stacktrace_convert: impl FnOnce(&S1) -> S2,
    ) -> GenericEventCore<S2> {
        GenericEventCore {
            t: r.t.clone(),
            stacktrace: stacktrace_convert(r.stacktrace),
            eff: r.eff,
            _phantom: PhantomData,
        }
    }
}

impl From<Event> for EventCore {
    fn from(value: Event) -> Self {
        EventCore {
            t: value.t,
            _phantom: PhantomData,
            stacktrace: value.stacktrace,
            eff: Eff::from(value.m.as_ref()),
            // addr: value.m.as_ref().map(|m| m.addr().to_owned()),
            // rval: value.m.as_ref().map(|m| m.loaded_value()).flatten(),
        }
    }
}

impl<'a> From<&'a Event> for EventCoreRef<'a> {
    fn from(value: &'a Event) -> Self {
        EventCoreRef {
            t: &value.t,
            _phantom: PhantomData,
            stacktrace: &value.stacktrace,
            eff: Eff::from(value.m.as_ref()),
            // addr: value.m.as_ref().map(MemoryAccess::addr),
            // rval: value.m.as_ref().map(|m| m.loaded_value()).flatten(),
        }
    }
}

impl<'a, T: std::fmt::Display> std::fmt::Display for GenericEventCoreRef<'a, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{{\n")?;
        write!(f, "  t: {},\n", self.t)?;
        write!(f, "  st: {},\n", self.stacktrace)?;
        // write!(f, "  loc: {:?},\n", self.addr)?;
        write!(f, "  eff: {:?},\n", self.eff)?;
        write!(f, "}}")?;
        Ok(())
    }
}

impl Event {
    /// Compares the equality of two events, *without* cnt.
    ///
    /// Theoretically, two events are equivalent, if and only if, the
    /// local behavior of e1.id is not changed at all when e1 is
    /// replaced by e2 and e1.id == e2.id.
    ///
    /// Note that, `cnt` is removed from the equality because cnt is
    /// only meta-level information, and it is not part of the formal
    /// definition of the event.
    pub fn equals(&self, other: &Self) -> bool {
        EventCoreRef::from(self) == EventCoreRef::from(other)
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
            "event - tid: {}, clk: {}, {} x pc: {}, cat: {}, eff: {:?}\n[{}]\n{}",
            self.t.id,
            self.clk,
            self.cnt,
            self.t.pc,
            self.t.cat,
            Eff::from(self.m.as_ref()),
            self.stacktrace
                .0
                .iter()
                .skip(1)
                .map(|x| x.to_string())
                .collect::<Vec<_>>()
                .join(","),
            self.instruction().display()?
        );
        for (i, frame) in self.stacktrace.0.iter().skip(1).rev().enumerate() {
            let call_insn = Instruction {
                path: PathBuf::from(&frame.fname),
                offset: frame.caller_pc.as_map_address().offset,
            };
            if let Ok(line) = call_insn.display_source() {
                result.push_str(&format!(
                    "--> [{}] Called by {}\n{}",
                    i + 1,
                    line.replace("\n", " "),
                    call_insn.display_assembly().unwrap_or("".to_string()),
                ));
            } else {
                result.push_str(&format!("--> [{}] Called by {}\n", i + 1, frame));
            }
        }
        Ok(result)
    }
}

impl std::fmt::Display for Event {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let ecore = EventCoreRef::from(self);
        write!(f, "{}x{}", ecore, self.cnt)
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
    pub clk: Clock,
}

impl PartialEq for PrimitiveConstraint {
    fn eq(&self, other: &Self) -> bool {
        self.equals(other)
    }
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

    /// An identifier for this constraint. It should be globally unique.
    pub id: usize,
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

pub unsafe fn sized_read(addr: u64, size: usize) -> u64 {
    fence(Ordering::SeqCst);

    let value = match size {
        1 => std::ptr::read_unaligned(addr as *const u8) as u64,
        2 => std::ptr::read_unaligned(addr as *const u16) as u64,
        4 => std::ptr::read_unaligned(addr as *const u32) as u64,
        8 => std::ptr::read_unaligned(addr as *const u64),
        _ => panic!("invalid size"),
    };

    value
}
