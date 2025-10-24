#  [lotto](README.md) / region_atomic.h
_The atomic region interface._ 

The (non)atomic region enables/disables task preemptions. The default state (outside the region) is controlled by the CLI flag `--preemptions-off`. If the flag is set, preemptions are disabled and nonatomic regions make a difference by allowing them. Otherwise, atomic regions can be used to disable preemptions locally. 

---
# Macros 

| Macro | Description |
|---|---|
| [lotto_atomic](region_atomic.h.md#macro-lotto_atomic) | Define an atomic region.  |
| [lotto_nonatomic](region_atomic.h.md#macro-lotto_nonatomic) | Define a nonatomic region.  |

##  Macro `lotto_atomic`

```c
lotto_atomic(...)
```

 
_Define an atomic region._ 



##  Macro `lotto_nonatomic`

```c
lotto_nonatomic(...)
```

 
_Define a nonatomic region._ 



---
# Functions 

| Function | Description |
|---|---|
| [lotto_region_atomic_enter](region_atomic.h.md#function-lotto_region_atomic_enter) | Enter the atomic region.  |
| [lotto_region_atomic_leave](region_atomic.h.md#function-lotto_region_atomic_leave) | Leave the atomic region.  |
| [lotto_region_atomic_enter_cond](region_atomic.h.md#function-lotto_region_atomic_enter_cond) | Enter the atomic region if the condition is met.  |
| [lotto_region_atomic_leave_cond](region_atomic.h.md#function-lotto_region_atomic_leave_cond) | Leave the atomic region if the condition is met.  |
| [lotto_region_atomic_enter_task](region_atomic.h.md#function-lotto_region_atomic_enter_task) | Enter the atomic region if the executing task id matches the argument.  |
| [lotto_region_atomic_leave_task](region_atomic.h.md#function-lotto_region_atomic_leave_task) | Leave the atomic region if the executing task id matches the argument.  |
| [lotto_region_nonatomic_enter](region_atomic.h.md#function-lotto_region_nonatomic_enter) | Enter the nonatomic region.  |
| [lotto_region_nonatomic_leave](region_atomic.h.md#function-lotto_region_nonatomic_leave) | Leave the nonatomic region.  |
| [lotto_region_nonatomic_enter_cond](region_atomic.h.md#function-lotto_region_nonatomic_enter_cond) | Enter the nonatomic region if the condition is met.  |
| [lotto_region_nonatomic_leave_cond](region_atomic.h.md#function-lotto_region_nonatomic_leave_cond) | Leave the nonatomic region if the condition is met.  |
| [lotto_region_nonatomic_enter_task](region_atomic.h.md#function-lotto_region_nonatomic_enter_task) | Enter the nonatomic region if the executing task id matches the argument.  |
| [lotto_region_nonatomic_leave_task](region_atomic.h.md#function-lotto_region_nonatomic_leave_task) | Leave the nonatomic region if the executing task id matches the argument.  |

##  Function `lotto_region_atomic_enter`

```c
static void lotto_region_atomic_enter(void)
``` 
_Enter the atomic region._ 



##  Function `lotto_region_atomic_leave`

```c
static void lotto_region_atomic_leave(void)
``` 
_Leave the atomic region._ 



##  Function `lotto_region_atomic_enter_cond`

```c
static void lotto_region_atomic_enter_cond(bool cond)
``` 
_Enter the atomic region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_atomic_leave_cond`

```c
static void lotto_region_atomic_leave_cond(bool cond)
``` 
_Leave the atomic region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_atomic_enter_task`

```c
static void lotto_region_atomic_enter_task(task_id task)
``` 
_Enter the atomic region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 




##  Function `lotto_region_atomic_leave_task`

```c
static void lotto_region_atomic_leave_task(task_id task)
``` 
_Leave the atomic region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 




##  Function `lotto_region_nonatomic_enter`

```c
static void lotto_region_nonatomic_enter(void)
``` 
_Enter the nonatomic region._ 



##  Function `lotto_region_nonatomic_leave`

```c
static void lotto_region_nonatomic_leave(void)
``` 
_Leave the nonatomic region._ 



##  Function `lotto_region_nonatomic_enter_cond`

```c
static void lotto_region_nonatomic_enter_cond(bool cond)
``` 
_Enter the nonatomic region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_nonatomic_leave_cond`

```c
static void lotto_region_nonatomic_leave_cond(bool cond)
``` 
_Leave the nonatomic region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_nonatomic_enter_task`

```c
static void lotto_region_nonatomic_enter_task(task_id task)
``` 
_Enter the nonatomic region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 




##  Function `lotto_region_nonatomic_leave_task`

```c
static void lotto_region_nonatomic_leave_task(task_id task)
``` 
_Leave the nonatomic region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 





---
