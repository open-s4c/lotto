#  [lotto](README.md) / region_preemption.h
_The preemption region interface._ 

The preemption region enables/disables task preemptions depending on the default state (controled by the the CLI flag `--region-preemption-default-off`). I.e., if the flag is unset, the regions behave atomically, otherwise tasks may preempt only inside regions. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_region_preemption_enter](region_preemption.h.md#function-lotto_region_preemption_enter) | Enter the preemption region.  |
| [lotto_region_preemption_leave](region_preemption.h.md#function-lotto_region_preemption_leave) | Leave the preemption region.  |
| [lotto_region_preemption_enter_cond](region_preemption.h.md#function-lotto_region_preemption_enter_cond) | Enter the preemption region if the condition is met.  |
| [lotto_region_preemption_leave_cond](region_preemption.h.md#function-lotto_region_preemption_leave_cond) | Leave the preemption region if the condition is met.  |
| [lotto_region_preemption_enter_task](region_preemption.h.md#function-lotto_region_preemption_enter_task) | Enter the preemption region if the executing task id matches the argument.  |
| [lotto_region_preemption_leave_task](region_preemption.h.md#function-lotto_region_preemption_leave_task) | Leave the preemption region if the executing task id matches the argument.  |

##  Function `lotto_region_preemption_enter`

```c
void lotto_region_preemption_enter() __attribute__((weak))
``` 
_Enter the preemption region._ 



##  Function `lotto_region_preemption_leave`

```c
void lotto_region_preemption_leave() __attribute__((weak))
``` 
_Leave the preemption region._ 



##  Function `lotto_region_preemption_enter_cond`

```c
void lotto_region_preemption_enter_cond(bool cond) __attribute__((weak))
``` 
_Enter the preemption region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_preemption_leave_cond`

```c
void lotto_region_preemption_leave_cond(bool cond) __attribute__((weak))
``` 
_Leave the preemption region if the condition is met._ 




**Parameters:**

- `cond`: condition 




##  Function `lotto_region_preemption_enter_task`

```c
void lotto_region_preemption_enter_task(task_id task) __attribute__((weak))
``` 
_Enter the preemption region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 




##  Function `lotto_region_preemption_leave_task`

```c
void lotto_region_preemption_leave_task(task_id task) __attribute__((weak))
``` 
_Leave the preemption region if the executing task id matches the argument._ 




**Parameters:**

- `task`: task id 





---
