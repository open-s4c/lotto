#  [lotto](README.md) / await.h
_Await changes to a memory address._ 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_await](await.h.md#function-lotto_await) | Request lotto to suspend the current task until a write to `addr` is detected.  |

##  Function `lotto_await`

```c
static void lotto_await(void *addr)
``` 
_Request lotto to suspend the current task until a write to_ `addr` _is detected._ 


Execution may continue sporadically even if no write was detected, e.g. if no tasks are ready. If the lotto `await` handler is not available or disabled, this annotation will have no effect. The function call itself is non-blocking and is just a hint to the scheduler to ignore this thread until a write is detected.



**Parameters:**

- `addr`: addr lotto should watch for writes 





---
