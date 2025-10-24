#  [lotto](README.md) / qemu.h
_Utilities to instrument QEMU guests._ 

The functions in this header should only be used in the context of QEMU. They will issue a fake opcode that is caught by Lotto and handled accordingly. Calling these functions in programs running natively on the host is undefined behavior. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_fail](qemu.h.md#function-lotto_fail) | Aborts Lotto's execution and signals a failure.  |
| [lotto_halt](qemu.h.md#function-lotto_halt) | Halts Lotto's execution and signals success.  |
| [lotto_fine_capture](qemu.h.md#function-lotto_fine_capture) | Sets QLotto plugin to inject fine-grained capture points.  |
| [lotto_coarse_capture](qemu.h.md#function-lotto_coarse_capture) | Sets QLotto plugin to inject grained-grained capture points.  |

##  Function `lotto_fail`

```c
static void lotto_fail(void)
``` 
_Aborts Lotto's execution and signals a failure._ 



##  Function `lotto_halt`

```c
static void lotto_halt(void)
``` 
_Halts Lotto's execution and signals success._ 



##  Function `lotto_fine_capture`

```c
void lotto_fine_capture()
``` 
_Sets QLotto plugin to inject fine-grained capture points._ 


This function can be called recursively. An equal number of `lotto_coarse_capture` has to be called to balance them out. 


##  Function `lotto_coarse_capture`

```c
void lotto_coarse_capture()
``` 
_Sets QLotto plugin to inject grained-grained capture points._ 


This function cancels the effect of `lotto_fine_capture` if called the same number of consecutive times. 



---
