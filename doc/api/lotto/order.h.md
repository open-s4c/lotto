#  [lotto](README.md) / order.h
_The ordering points interface._ 

Ordering points are events which must be executed in the order defined by their arguments. Orders start with 1, must be unique, and must not contain gaps. When an out of order point is about to be executed, the task is blocked until the previous point is executed. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_order](order.h.md#function-lotto_order) | Inserts an ordering point.  |
| [lotto_order_cond](order.h.md#function-lotto_order_cond) | Inserts an ordering point if the condition is satisfied.  |

##  Function `lotto_order`

```c
void lotto_order(uint64_t order) __attribute__((weak))
``` 
_Inserts an ordering point._ 




**Parameters:**

- `order`: order 




##  Function `lotto_order_cond`

```c
void lotto_order_cond(bool cond, uint64_t order) __attribute__((weak))
``` 
_Inserts an ordering point if the condition is satisfied._ 




**Parameters:**

- `cond`: condition 
- `order`: order 





---
