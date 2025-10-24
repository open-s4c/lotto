#  [lotto](README.md) / priority.h
_The priority interface._ 

User priorities override strategy decision: the strategy applies to available tasks with the highest user priority. Initially, all priorities are zero. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_priority](priority.h.md#function-lotto_priority) | Sets the task priority.  |
| [lotto_priority_cond](priority.h.md#function-lotto_priority_cond) | Sets the task priority if the condition is satisfied.  |
| [lotto_priority_task](priority.h.md#function-lotto_priority_task) | Sets the task priority if the executing task id is equal to `task`.  |

##  Function `lotto_priority`

```c
void lotto_priority(int64_t priority) __attribute__((weak))
``` 
_Sets the task priority._ 




**Parameters:**

- `priority`: priority 




##  Function `lotto_priority_cond`

```c
void lotto_priority_cond(bool cond, int64_t priority) __attribute__((weak))
``` 
_Sets the task priority if the condition is satisfied._ 




**Parameters:**

- `priority`: priority 
- `cond`: condition 




##  Function `lotto_priority_task`

```c
void lotto_priority_task(task_id task, int64_t priority) __attribute__((weak))
``` 
_Sets the task priority if the executing task id is equal to_ `task`_._ 




**Parameters:**

- `priority`: priority 
- `task`: task id 





---
