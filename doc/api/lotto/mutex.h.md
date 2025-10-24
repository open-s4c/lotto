#  [lotto](README.md) / mutex.h
_Lotto-based mutex implementation._ 

Mutex provides no FIFO guarantees on the order of the waiters being awaken. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_mutex_acquire](mutex.h.md#function-lotto_mutex_acquire) | Acquires a mutex.  |
| [lotto_mutex_tryacquire](mutex.h.md#function-lotto_mutex_tryacquire) | Tries to acquire a mutex.  |
| [lotto_mutex_release](mutex.h.md#function-lotto_mutex_release) | Releases mutex.  |

##  Function `lotto_mutex_acquire`

```c
static void lotto_mutex_acquire(void *addr)
``` 
_Acquires a mutex._ 




**Parameters:**

- `addr`: opaque mutex identifier 




##  Function `lotto_mutex_tryacquire`

```c
static int lotto_mutex_tryacquire(void *addr)
``` 
_Tries to acquire a mutex._ 




**Parameters:**

- `addr`: opaque mutex identifier 


**Returns:** 0 on success, otherwise a non-zero value 



##  Function `lotto_mutex_release`

```c
static void lotto_mutex_release(void *addr)
``` 
_Releases mutex._ 




**Parameters:**

- `addr`: opaque mutex identifier 





---
