#  [lotto](README.md) / yield.h
_The yield interface._ 

The yield interface can be used for either enforcing preemptions or adding change points. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_yield](yield.h.md#function-lotto_yield) | Preempt to another available task.  |

##  Function `lotto_yield`

```c
int lotto_yield(bool advisory) __attribute__((weak))
``` 
_Preempt to another available task._ 


An advisory yield is interpreted as a change point, i.e., the preemption decision is left to the strategy and may include the current task. Otherwise, the current task is excluded from the pool of next tasks.



**Parameters:**

- `advisory`: whether the yield is advisory 


**Returns:** zero 




---
