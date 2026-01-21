# Introduction

Creation of concurrent software is a nontrivial task which is prone to hard to find bugs. Developers have to find a trade-off between slow overprotecting code patterns and more performant but possibly wrong optimizations.

Lotto is a framework for reproduction and deterministic replay of concurrency bugs. It aims for supporting both user-space (PLotto) and kernel-space (QLotto) scenarios as well as providing an extensible platform to accomodate specific user needs with domain-specific plugins.

# Architecture

Lotto can be seen as consisting of three top-level components:

- CLI
- frond-ends

The CLI provides the user interface for configuring and running the system under test (SUT). Front-ends connect to SUT and are responsible for passing necessary information to the engine and executing its plan. ?? is an external tool for data replay. The top-level components share some modules of Lotto as shown below.

```
+---------+
| dlotto  | <---------------- containerized Lotto (Alpine Linux + Docker)
+---------+
| qlotto  | <---------------- kernel-space Lotto (Qemu + plugins)
+---------+
| plotto  | <---------------- user-space Lotto
+---------+  +--------------- UI
| runtime |  V
+---------+-----+
| engine  | cli |
+---------+-----+   +--------- syscall record/replay
|    states     |   V
+---------------+------+
|    brokers    |  ??  |
+---------------+------+
|         base         |
+----------------------+
|         sys          |
+----------------------+
```

Sys and base provide a library for other components. ?? uses just these two modules. Brokers and states are specific to Lotto engine. They are still used by the CLI for setting up the run. The engine sees SUT on the abstract level through the runtime. The runtime provides a unified interface for all domain-specific front-ends. The simplest frontend is PLotto. It contains basic interceptors for pthread and TSAN allowing most basic user-space applications compiled with TSAN to be run with Lotto. QLotto is a specific case of PLotto applied to a modified version of Qemu enriched with Lotto-aware plugins. This front-end allows testing deterministic replay of kernel-space applications run inside Qemu images. QLotto differs from plain PLotto run with vanilla Qemu by guest-aware interceptors. As a proof of concept, this sequence continues with DLotto. This frontend consists of QLotto running Alpine Linux with Docker inside. Theoretically, it allows running any uninstrumented application with Lotto.

# Components

## `sys`
`sys` is a wrapper around standard library functions and syscalls. It serves two main purposes. Since Lotto utilizes the so-called ``LD_PRELOAD trick'' for instrumentation, it declares functions with the same names as the ones it aims to intercept. Hence, if the original functions need to be called from the runtime, the name clash has to be resolved by symbol lookup. The `sys` component caches the real function addresses in `sys_`-prefixed wrapper functions which avoid the lookup overhead. While the runtime has capture guards which avoid recursive interception, it is still recommented to use `sys_` versions of functions accross the engine to avoid performance overhead coming from accessing the guards. Another important role is SUT isolation. Currently, `sys_malloc()` in Lotto uses its own memory manager instead of simply redirecting calls to the real `malloc()`. It is done to preserve memory allocation determinism of SUT under the assumption that the same sequence of memory management calls returns the same pointers. Thus, Lotto may allow itself to have varying internal dynamic memory use on record and replay while avoiding potential control flow deviations in SUT, e.g., due to identity hashes being used.

> TODO: move what does not fit into the base (stream, plugin, etc.), extend sys

## `base`

`base` provides data structures and algorithms used by other components of Lotto. It is stateless.

> TODO: move memory map to `brokers`

## `brokers`
`brokers` is a collection of publish–subscribe communication channels shared between the CLI, engine, and stateful components shared between the CLI and engine stacks. Its main components are:

- state manager
- pubsub
- cat manager

### `statemgr`

The state manager registers serializable data structures and orchestrates their joint (un)marshalling. Its primary use is writing and reading record payload. Data structures with marshalling interfaces subscribe to a certain state type:

- config
- persistent
- final
- ephemeral

The first three types correspond to record types, i.e., they define where the state data appears in the trace. Ephemeral type is opaque and used for uniformity with states which do not need to be recorded but still have to perform initialization. The state manager assigns slots in the order of state registration, which determine the order of (de)serialization. Hence, the order of registration must be the same in both CLI and the engine across runs for traces to be compatible. Recorder and the CLI act as publishers when they write or read trace records.

> TODO: autogenerate slots
> TODO: enforce the same states

### `pubsub`

`pubsub` is a means of communication for informing about events inside Lotto called topics. These topics include but are not limited to

- engine start
- before and after state (de)serialization
- scheduling decision
- record reading
- trace saving
- replay end
- deadlock detection
- enforce violation
- timeout trigger

`pubsub` only supports topics to be published from inside the engine. Hence, all topics are published in a sequence. Subscribers are not allowed to modify published data but they do not have to appear only in the engine.

> TODO: remove handler-specific topics
> TODO: cleanup
> TODO: disable topics with CMake

### `catmgr`

Category manager provides an interface for extension of event categories. Plugins can request a fresh category id by providing a string with the new category name for pretty printing, its group(s), and a contract lambda. Category groups are used for domain-agnostic support of replay in the sequencer and contract checking in the engine. Contract lambdas optionally extend the contract with custom checks. Since both the engine and the CLI can access categories, their registration must happen in the same order. Category enforcer ensures that it is the case whenever a trace is involved.

> TODO: add possibility to extend the contract and category groups

## `states`
The `states` component is a collection of non-ephemeral `statemgr` subscribers. It is shared among CLI and the engine allowing the former to communicate with the latter through the trace.

> TODO: decide on what state set differences are acceptable

## `engine`
The Lotto engine is a domain-agnostic component seeing SUT execution on an abstract level. It provides an interface for communicating intercepted events and a plan. The entry interface checks the engine contract and delegates requests to its main component, the sequencer. The contract checks some invariants about its users and the sequencer output:

- no simultaneous capture/resume calls
- resume must be followed by capture and vice versa
- returns can happen only in detached tasks
- plan must match the category semantic

The plan consists of a sequence of actions, the order of which is predetermined by their enum bit (same as the order in this list):

- `WAKE` - lets the next task inside `BLOCK` or `YIELD` execute, `ANY_TASK` picks a task in `YIELD` nondeterministically
- `CALL` - performs a potentially blocking call
- `BLOCK` - blocks a task inside Lotto (equivalent to `YIELD` irresponsive to `WAKE` of `ANY_TASK`)
- `RETURN` - informs the engine that `CALL` finished
- `YIELD` - blocks the task inside Lotto, is subject to `WAKE` of `ANY_TASK`
- `RESUME` - informs the engine that the task resumed and continues execution
- `CONTINUE` - continue execution
- `SNAPSHOT` - takes a snapshot of the current state if the front-end supports it
- `SHUTDOWN` - ends the run prematurely

These actions are used to express the following base scenarios:

- `WAKE|YIELD|RESUME` - generic preemption
- `WAKE|BLOCK|RESUME` - preemption before blocking inside Lotto, e.g., for locks implemented in the engine
- `WAKE|CALL|RETURN|YIELD|RESUME` - preemption before external blocking call

All of these scenarios can be complemented with `SNAPSHOT` and `SHUTDOWN`. While the actions may seem independent of each other, the design of runtime is tailored for efficient execution of these three scenarios.

> TODO: add possibility to extend the contract when registering the category

### `sequencer`
The sequencer is responsible for connecting other engine components. It exposes three main interfaces:

- capture
- resume
- return

The capture interface processes the intercepted event. It lets the recorder load the next trace record, passes the information it has to the dispatcher, and creates a plan for its scheduling decision.

The resume interface informs the sequencer that some task continues its execution. The sequencer updates its internal information and lets the recorder create a trace record.

The return interface is used by detached tasks upon returning from a blocking call. The sequencer adds all returns into a queue which is processed during the next capture event. Unlike the other two interfaces, this interface can be called at any point of time by any detached task, i.e, it is concurrent.

These interfaces together with plan allow the engine to process SUT on an [abstract level](#system-under-test-sut).

### `dispatcher`
The dispatcher passes the event from the sequencer and passes it through the chain of handlers. If handlers have not decided on the next task, it picks a random task out of the remaining set. If handlers have run out of available tasks, `ANY_TASK` will be scheduled and resolved to some task nondeterministically by `runtime`.

### `handlers`
A handler is a logic unit implementing either a partial scheduling decision or information tracking. Handlers register themselves at the dispatcher and are called in a specific order determined by their slots. While the slot sequence is a plain structure, handlers can be classified as either belonging or relating to one of the following groups:

- populators: add tids to the tidset. After they're executed the tidset reaches its maximum size for a dispatched event.
- determinators: take care of nondeterministic events. After them a sensible input subset to the handler chain is deterministic.
- blockers/waiters: may strongly block the current task (either with WAIT or CALL)
- adjusters: influence performance of handlers which come after
- constrainers: constrain the schedule by making partial decisions
- strategies: make the final decision on the next task
- information collectors
- error detectors: terminate execution prematurely

The groups appear in their relative order in the handler chain. Handler functionality must be designed in such a way that it can fit in, before or after either one of the aforementioned groups and its position does not depend on any handlers other than explicit dependencies.

Handlers must follow some conventions:

- tidset should be extended only by populators
- boolean event fields must be updated monotonically (`true` to `false` is not allowed)
- selector must be set only once
- if `readonly` is set, the event must not be mutated
- if `skip` is set, the handler should reduce its functionality only to bookkeeping and avoiding deadlocks

> TODO: dynamic handler registration
> TODO: check handler contract in the dispatcher

### `recorder`
Recorder ensures timely loading of input records and creation of output records. For both actions it uses the state manager.

## `runtime`
Runtime provides a simple interceptor interface to front-ends and encapsulates plan execution. It provides layers of wrappers around basic capture, resume, and return interfaces of the engine. The purpose of these layers is to gradually encapsulate event passing and plan execution. The dependency graph is shown below.

```
               switcher   runtime -> sighandler
                  ^          |
                  |          V
interceptor -> mediator -> engine
```

The main runtime initializes the engine and enables the sighandler. The interceptor forwards the calls to the mediator, which passes the events to the engine and uses the switcher to execute some actions from the plan.

> TODO: be agnostic to pthread or put it as a limitation

### `mediator`
Mediator is the only part of runtime communicating with the engine and switcher directly. It passes captured event to the engine and is responsible for executing `WAKE`, `BLOCK`, `YIELD`, and `RESUME` actions. Since the mediator interfaces mirror the ones of the engine, and some actions like `CALL` can be executed by the outermost interceptor only, the plan execution is interrupted by returning back to the interceptor. The mediator communicates to the interceptor whether the latter needs to call the mediator again through its return values:

- `mediator_capture()` returns a boolean of whether `mediator_resume()` should be called by the interceptor
- `mediator_resume()` returns mediator status, which can be either
 - `OK` - no action needed
 - `SHUTDOWN` - terminate normally
 - `ABORT` - terminate with an error code

The interceptor must take these return values into account to accommodate a control flow suitable for plan execution.

Apart from being part of plan execution, the mediator also provides guards to prevent recursive interception. Lotto supports having interceptors on various levels of abstraction, which may lead to lower level interceptors being triggered inside a higher level intercepted function. Hence, the mediator supports guard and forgo interfaces which create a region within which the interceptors cannot reach the engine.

Since interceptor guarding is task-specific, the mediator has to store corresponding data. It utilizes thread-local storage (TLS) for creating and accessing the mediator object. The aforementioned interfaces are agnostic to the TLS implementation and always expect the mediator object to be provided. The object construction and access is implemented using pthread TLS and is isolated from the other interfaces to keep the pthread-specific code minimal.

### `switcher`
Switcher implements yield and wake primitives for controlling the schedule. Yield blocks the caller task until it is waked. The wake primitive marks the next thread, which does not have to be the caller. Once the waked tasks leaves its yield state, the mark is consumed. Therefore, the switcher contract is:

- there must be a return from yield between two wakes
- if a task can wake itself it must do it before it yields

A special case is when `ANY_TASK` is used in wake. Then the first yielded task which checks its wake condition will return, i.e., the choice will be done nondeterministically.

A special variant of yield is wait. It removes the caller from the `ANY_TASK` selection. Thus, strict blocking is implemented in lotto.

### `interceptor`
The interceptor encapsulate the mediator use to supply a concise interface for domain-specific interceptors. It simplifies communication with Lotto to two basic scenarios:

- interception before an event
- wrapping an external blocking event

The former scenario just puts `intercept_capture()` before the code executing the event. The event must not block. The latter case handles external blocking events by surrounding them with `intercept_before_call()` and `intercept_after_call()`. These calls create a region which is detached from Lotto, i.e., it runs in parallel with other tasks. Thus, to ensure deterministic replay there must be no race between blocking events.

### `sighandler`

The signal handler ensures graceful termination of Lotto when a signal is received. For each signal it issues a termination event which allows the engine to finalize the trace.

## `plotto`
PLotto is a collection of interceptors for pthread and TSAN. They are sufficient for basic user-space scenarios.

## `qlotto`
QLotto extends functionality of PLotto by making use of Qemu and dynamic instrumentation of guest VM instructions. This frontend provides a possibility of testing and replaying kernel code. On the high level, it is PLotto with a QLotto plugin being applied to a modified version of Qemu with plugins running SUT image. The PLotto plugin integrates Qemu plugin configurations into Lotto CLI.

### Qemu
Qemu code had to be adjusted to accommodate QLotto needs. The changes are kept minimal to reduce maintenance overhead when switching to another version of Qemu. Thus, the modifications fall in a few well-defined categories.

#### enable running Qemu with PLotto
Qemu uses its own futex implementation with `syscall()` which is currently not supported by PLotto. It was replace with an equivalent evec interface implemented in Lotto.

#### extend plugin API
QLotto plugins need more interaction with Qemu than standard Qemu plugins. Hence, Qemu source code had to be changed to enable the following:

- callback informing that a VCPU registered sharing the CPU data structure
- callbacks for tracking transitions between Qemu and the guest
- additional callbacks for performance analysis
- interface to request a snapshot from the plugin

While the changes cover passing more information to the plugins and letting plugins influence Qemu execution, they keep Qemu and plugin code decoupled allowing plain Qemu execution.

#### support guest instrumentation
QLotto allows the guest communicate events to Lotto engine by executing undefined instructions (UDFs). These instructions can be inserted both manually in the source code and during the translation. Qemu had to be changed to let callbacks process UDFs and not terminate when receiving a `SIGILL`.

### performance
The performance plugin is used to evaluate runtime productivity of QLotto. It represents sequential execution as a state machine switching between Qemu, the frontend plugin, and Lotto engine. Thus, the performance measurements provide an overview of overhead added by various QLotto components.

### `frontend`
The frontend is a Qemu-specific interceptor extension of Lotto. It registers callbacks in Qemu which create contexts for received events and pass them to the interceptor. The context is guest-aware and includes not only the VCPU id but also the guest thread id.

### `gdb`
The gdb plugin enables debugging with QLotto. It runs during both record and replay to ensure replay determinism. Unlike the native Qemu gdb server, the QLotto gdb server is invisible for Lotto and does not break replay when a gdb client connects and uses the gdb server.

### `snapshot`
The snapshot plugin is responsible for taking snapshots when Qemu is run without Lotto. This feature enables faster debug loop in scenarios where the bug can be reproduced with plain Qemu and the execution run takes too long with QLotto:

```
               Qemu execution
image -+----...----+-------...-----+------> bug
       |           |               |
       V           V               V
   snapshot 1  snapshot i      snapshot n
                   |
                   |     QLotto stress
                   +----------------------> bug + Lotto trace
```

At first, the bug has to be reproduced with plain Qemu. The snapshot plugin creates snapshots regularly. Then the suffix of interest has to be determined by the user. The longer the suffix is, the more time it will take for QLotto to execute it. On the other hand a too short suffix may miss the point when the root cause of the bug happens. Then, the snapshot can be used with Lotto stress to reproduce the bug with QLotto. Once the bug is hit, Lotto trace is generated and the suffix can be deterministically replayed.

### Changes to the guest
The guest can be manually intercepted to make Lotto aware of certain events. For instance, annotating all lock interfaces would allow Lotto to detect deadlocks. QLotto provides convenience headers for easy insertion of capture points.

## `dlotto`
Scripts and images.

## `cli`
The CLI allows launching SUT with Lotto using terminal. It makes use of `states` and `brokers` to configure Lotto engine and contains internal interfaces for flags, trace, and execution which minimize boiler-plate code when developing new CLI commands

### commands
Lotto supports several commands for both basic runs and deeper analysis of the program. The base commands are

- run - perform a single run without producing a trace
- record - perform a single run and produce a trace
- stress - perform runs until a bug is hit or the number of runs is exhausted and produce the trace
- replay - replay a trace
- debug - start replay in gdb extended with Lotto-specific convenience commands

These commands are sufficient for simple debug scenarios. On top of them, Lotto accommodates more advanced use cases

- explore - exhaustive search for a bug in all schedules
- inflex - trim the trace to the inflection point

To implement a new command, one needs to register it. Optionally, a command can define its own flags. Many commonly used flags are provided by the flag manager as macros. Then the command can set the environment variables, access the state, execute SUT, and inspect its exit code.

### `driver`
The driver provides an interface `execute()` for running SUT. It causes the flag manager to execute flag callbacks and initializes the input trace before starting SUT. It is the only component which can launch SUT.

> TODO: Rename `exec` into `driver`

### `flagmgr`
The flags are registered, parsed, and propagated by the flag manager. When a new flag is registered, it needs to provide

- short option
- long option
- help description
- default value
- callback
- parser and pretty printer
- variable name

The mandatory components are long or short option and the default value. The options define how the option appears in the interface (`-s` and/or `--long`). The help description is shown in the message listing all flags. The default value is applied when the option is not set. The supported values are boolean, integer, and string. The callback is executed before each run of SUT. It is suitable for passing configuration to engine states. The parser and pretty printer allow to specify a different format for flag values. E.g., boolean flags do not have an argument by default but when provided the parser-printer pair, they would accept a pretty string value and internally convert it into boolean, e.g., `--boolean-flag enabled`. Variable name is needed for flags which need to be used in the code. It is an alternative to the callback and it convenient to use in command implementation.

### `flags`
This component mirrors `states` and includes all engine flags which are part of the core code base. These flags define callbacks which set a corresponding state.

# System under test (SUT)

Lotto sees SUT as a set of parallel tasks, each of which follow a specific life cycle ensured by the runtime:

```
                                                                                                  return
                                                                                   calling ------------------> returned
                                                                                     ^                              |
                                                                                     |          waiting CAT         |
                                                                    capture CAT_CALL |              ^ |             | resume CAT_CALL
                                                                                     |  capture CAT | | resume CAT  |
                                                                                     |              | |             |
                                                                                     +----------+   | |   +---------+
                                                                                                 \  | |  /
          resume CAT_NONE            capture CAT_TASK_INIT                 resume CAT_TASK_INIT   \ | V V  capture TASK_FINI
creating -----------------> created -----------------------> initializing ----------------------> running -------------------> finished
                                                                                                    | ^
                                                                                        capture CAT | | resume CAT
                                                                                                    | |
                                                                                                captured CAT
```

When a task is created it resumes from an opaque category `NONE` and intercepts a `TASK_INIT` event. At this point of time the task can be preempted, i.e., a newly created task does not have to make any progress before it yields for the first time. After the task resumes, it enters a steady loop which processes all real events. Depending on the category of the event, it manifests either as a capture-resume pair or as a capture-return-resume triple. The former mode fits all events for which the schedule can be handled fully by Lotto either through blocking in the `waiting` state or through preemting in the `captured` state. The triple scheme is a domain-agnostic way of handling arbitrary blocking calls, i.e., it is an alternative to the `waiting` loop. In the `calling` state the task is detached from Lotto and runs in parallel until it enters the `returned` state. Then the behavior is the same as in the `captured` state. When the task finishes, it informs Lotto about its termination with the `TASK_FINI` event.

With the SUT task state machine in mind, one can see the whole SUT as a network of task automata. It must conform to the following invariants:

- `return` can happen at any time (nondeterministic blocking call termination)
- only one task can be a `resume` target state (sequential execution)
- `running` state eventually `capture`s (instrumentation completeness)
- as long as there is a task not in `creating`, `waiting`, or `calling` state, either its transition may be taken (Lotto liveness)

The liveness condition can be read as ``Lotto blocks ensuring sequentiality cannot lead to a deadlock.'' Since SUT is not necessarily deadlock-free, the strongest liveness guarantee Lotto can provide is not introducing new deadlocks. A genuine SUT deadlock would manifest in all active tasks being in blocking calls which are represented by `waiting` and `calling` states. Hence, their `return` and `resume` transitions are controlled by SUT. The `creating` state represents tasks before their creation, hence it it also controlled by SUT. All other `resume` sources are existing tasks ready to be scheduled. The `TASK_INIT` event is created by Lotto during task initialization. The `running` state cannot be deadlocking if the instrumentation is complete.

TBC

# Data flows

## `cli` -> `engine`

CLI communicates with the engine through environment variables and the trace.

```
                     S
         cli ~~~~~~~~~~~~~~~~~~~> SUT
   opts              S
    |                S
    V                S
 flagmgr -> command  S          engine
    |          |     S            ^
    |          |     S            |
    |          |     S     +------+-----+
    V          V     S     |            |
  states    setenv() -> getenv()      states
    |                S                  ^
    V                S                  |
 statemgr            S               statemgr
    |                S                  ^
    |                S                  |
    |                S               recorder
    |                S                  ^
    V                S                  |
input.trace ----------------------------+
                     S
```

Command-line options are parsed by the flag manager and either made accessible to commands or go to the receiver states directly. The states are then marshalled into the initial trace which is then accessed by the engine from the SUT process. This is the preferable way of passing information to the engine. In rare cases, the command can use environment variables as an alternative means of communication. This is allowed in the following scenarios:

- the information needs to be accessed before the first record is read (e.g., the trace location or logging parameters)
- replay behavior has to differ (e.g., replaying with a different record granularity)

> TODO: read config in the constructor to eliminate (1)
> TODO: ensure and document correct order of component linking

## `engine` -> `cli`

The only supported way of getting information from SUT into CLI is the output trace.

```
         S
    cli ~~~~>  SUT
command  S    engine
   ^     S      |
   |     S      V
 states  S    states
   ^     S      |
   |     S      V
statemgr S   statemgr
   ^     S      |
   |     S      V
   |     S   recorder
   |     S      |
   |     S      V
   +------ output.trace
         S
```

## Adjustment

Sometimes a parameter needs to be adjusted to a more optimal value based on the information from the previous runs, i.e., the CONFIG from the next run may depend on some FINAL state of the current run. One can implement this data flow in CLI with pubsub:

```
                           S
                  cli      S     SUT
                CONFIG ~~~~~> input.trace
                           S      | run i
                           S      V
              output.trace <~~~~~ FINAL
          adjust() |       S
                   V       S
                 FINAL     S
pubsub after FINAL |       S
                   v       S
                 CONFIG ~~~~> input.trace
                           S      | run i+1
                           S      V
```

The final state is generated during the run and stored in the trace. In the CLI, the command can call `adjust()` which reads the final trace. This causes the after-unmarshal event to be published allowing the subscriber to incorporate the FINAL state into the next CONFIG state. Thus, support for new adjustable components can be added to existing commands using `adjust()`.

## `runtime` <-> `engine`
TBC
```
              context
         ----------------->
mediator  engine_capture()  engine
         <-----------------
               plan
```

## Inside `engine`
> TODO: dynamic registration of events

# User interface
# Developer interface
# Tests
# Plugins
# Performance considerations
# Compatibility considerations
# Glossary
