### Filename conventions

A filename reflects the pattern of memory accesses 
and reconstruction logs used in the test.
It contains a mandatory test family, a mandatory test pattern, 
and an optional test suffix.
```
<family>.<pattern>(.<suffix>)*.c
```

A family describes the sequence of memory accesses in each thread.
If the same sequence is executed multiple times,
then the number of repetitions can be represented by an integer number.

A pattern describes the sequence of reconstruction logs,
in addition to the sequence of memory accesses.

An optional suffix defines additional configurations that 
differentiate a particular test from the other tests in the 
same family having the same log pattern.
For example, a suffix can contain a value used in the assertion,
or a description of the scheduling in the log file.

Filename format
```
<filename>              : <family>.<pattern>(.<suffix>)*.c
<family>                : <thread_family>(_<thread_family>)*
<pattern>               : <thread_pattern>(_<thread_pattern>)*
<thread_family>         : <instructions_family>-<count>(-<instructions_family>-<count>)*
<thread_pattern>        : <instructions_pattern>-<count>(-<instructions_pattern>-<count>)*
<instructions_family>   : (R | W | Ra | Wa | Rmw)+
<instructions_pattern>  : (R | W | Ra | Wa | Rmw | L | D)+
<count>                 : [1-9]([0-9])*
<suffix>                : [a-zA-Z0-9]+
```

Instruction codes
- `R`   - read (non-atomic)
- `W`   - write (non-atomic)
- `Ra`  - atomic read
- `Wa`  - atomic write
- `Rmw` - atomic read-modify-write
- `L`   - reconstruction log (without data)
- `D`   - reconstruction log (with data)
