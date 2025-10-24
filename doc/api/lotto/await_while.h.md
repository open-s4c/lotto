#  [lotto](README.md) / await_while.h
_Prevent user annotated spin loops from unnecessary iterations._ 

To use this feature, replace `while` with `await_while`, `do {} while()` with `await_do {} while_await()`, and `break` with `await_break`.

These macros are compatible with those in vsync/await_while.h and serve as a drop-in replacement.

## Usage guidelines


- Use `break`s and `await_break`s consistently with the loop annotations.
- Use atomics for the loop conditions
- Avoid adding unnecessary assertions inside spin loops


If you are using libvsync, force this header to be included in the compilation units by passing the following option to the compiler: `-include path/to/lotto/await_while.h`.

## Implementation details

When a loop is annotated with the macros in this file, Lotto can automatically detect whether the spinloop should be blocked. This works for:


- Write-reads in the same spin loop;
- Reading from a value modified outside the loop;
- Reading from a value modified in a blocked spin loop;


The handler records every read and write that occurs inside the loop and watchs for writes from outside. If there is a write from outside, we can't safely block the spin loop; if the spin loop reads and then writes to the same address, we also can't safely block it. There are some more complicated cases that can be found in the rust code documentation.

There are a few points to be careful:


- Local variables can be compiled to use only a register, so again, please use ATOMICS for loop conditions!
- It looks for **ALL** the reads/writes, so avoid using extra `assert`s. If needed, add it to a lotto_disable region: "lotto/unsafe/disable.h".
- CMP_XCHG counts as a Read (or a Read+Write) only for the modified atomic. The local variable is ignored.


The implementation in Lotto might deadlock under any of these conditions:
- The loop condition uses local variable as a spin loop condition, which maybe is not intercepted by TSAN.
- The compiler optimizes a variable in the loop condition into a register.
- The compiler optimizes a write into a move, which is not intercepted.


## Reference

Based on the following paper, but also allowing writes in the spin loop.


## References:
 [https://arxiv.org/abs/2012.01067](https://arxiv.org/abs/2012.01067) 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_spin_start](await_while.h.md#function-lotto_spin_start) | Marks the begging of a new spin loop `iteration`.  |
| [lotto_spin_end](await_while.h.md#function-lotto_spin_end) | Marks the end of a spin loop `iteration`.  |

##  Function `lotto_spin_start`

```c
static void lotto_spin_start()
``` 
_Marks the begging of a new spin loop_ `iteration`_._ 


Do not use this function directly, instead use the available macros. 


##  Function `lotto_spin_end`

```c
static void lotto_spin_end(uint32_t cond)
``` 
_Marks the end of a spin loop_ `iteration`_._ 


Do not use this function directly, instead use the available macros.



**Parameters:**

- `cond`: If 1 the spin_loop ended successfully, else it will continue 





---
