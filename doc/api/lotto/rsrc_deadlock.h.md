#  [lotto](README.md) / rsrc_deadlock.h
---
# Functions 

| Function | Description |
|---|---|
| [lotto_rsrc_acquiring](rsrc_deadlock.h.md#function-lotto_rsrc_acquiring) | Informs that the current task is acquiring the resource.  |
| [lotto_rsrc_tried_acquiring](rsrc_deadlock.h.md#function-lotto_rsrc_tried_acquiring) | Informs that the current task has tried to acquire the resource.  |
| [lotto_rsrc_released](rsrc_deadlock.h.md#function-lotto_rsrc_released) | Informs that the current task has released the resource.  |

##  Function `lotto_rsrc_acquiring`

```c
static void lotto_rsrc_acquiring(void *addr)
``` 
_Informs that the current task is acquiring the resource._ 


This marker is placed **before** the task actually acquires the resouce, for example, a just before `mutex_lock()`. The actual resource aquisition might block. For non-blocking resource acquisition, for example, `mutex_trylock()`, refer to `lotto_rsrc_tried_acquiring`.



**Parameters:**

- `addr`: opaque resource identifier 




##  Function `lotto_rsrc_tried_acquiring`

```c
static bool lotto_rsrc_tried_acquiring(void *addr, bool success)
``` 
_Informs that the current task has tried to acquire the resource._ 


This marker is placed **after** the task tried to acquire a resource, for example, after `mutex_trylock()`. Even though a trylock cannot cause a hang, the algorithm needs to record the information that the resource was actually acquired to be able to detect deadlocks in other conditions.



**Parameters:**

- `addr`: opaque resource identifier 
- `success`: whether task succeeded acquiring or not 


**Returns:** value of success parameter 



##  Function `lotto_rsrc_released`

```c
static void lotto_rsrc_released(void *addr)
``` 
_Informs that the current task has released the resource._ 


This marker should be placed **after** the resource has been released.



**Parameters:**

- `addr`: opaque resource identifier 





---
