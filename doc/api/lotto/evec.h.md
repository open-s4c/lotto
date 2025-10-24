#  [lotto](README.md) / evec.h
_Event counters for low level synchronization._ 

Tasks can wait for a condition to be true and block if the condition is not true. Otherwise, the task can cancel the waiting. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_evec_prepare](evec.h.md#function-lotto_evec_prepare) | Registers intent to wait for event.  |
| [lotto_evec_wait](evec.h.md#function-lotto_evec_wait) | Waits for event.  |
| [lotto_evec_timed_wait](evec.h.md#function-lotto_evec_timed_wait) | Waits for event or the deadline.  |
| [lotto_evec_cancel](evec.h.md#function-lotto_evec_cancel) | Cancels intent to wait for event.  |
| [lotto_evec_wake](evec.h.md#function-lotto_evec_wake) | Wakes `cnt` waiters of the event.  |
| [lotto_evec_move](evec.h.md#function-lotto_evec_move) | Move all waiters of `src` address to `dst address.  |

##  Function `lotto_evec_prepare`

```c
static void lotto_evec_prepare(void *addr)
``` 
_Registers intent to wait for event._ 




**Parameters:**

- `addr`: opaque event identifier 




##  Function `lotto_evec_wait`

```c
static void lotto_evec_wait(void *addr)
``` 
_Waits for event._ 




**Parameters:**

- `addr`: opaque event identifier 




##  Function `lotto_evec_timed_wait`

```c
static enum lotto_timed_wait_status lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
``` 
_Waits for event or the deadline._ 




**Parameters:**

- `addr`: opaque event identifier 
- `abstime`: deadline timespec


**Returns:** `TIMED_WAIT_OK` if returned by wake, `TIMED_WAIT_TIMEOUT` if returned by timeout, `TIMED_WAIT_INVALID` if `abstime` is invalid 



##  Function `lotto_evec_cancel`

```c
static void lotto_evec_cancel(void *addr)
``` 
_Cancels intent to wait for event._ 




**Parameters:**

- `addr`: opaque event identifier 




##  Function `lotto_evec_wake`

```c
static void lotto_evec_wake(void *addr, uint32_t cnt)
``` 
_Wakes_ `cnt` _waiters of the event._ 


If there is no waiter, the wake is ignored.



**Parameters:**

- `addr`: opaque event identifier 
- `cnt`: maximum number of waiters to wake 




##  Function `lotto_evec_move`

```c
static void lotto_evec_move(void *src, void *dst)
``` 
_Move all waiters of_ `src` _address to `dst address._ 




**Parameters:**

- `src`: opaque event identifier from which all waiters will be moved. 
- `dst`: opaque event identifier to which all waiters will be moved. 





---
