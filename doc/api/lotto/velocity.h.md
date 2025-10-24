#  [lotto](README.md) / velocity.h
_The task velocity interface._ 

User task velocities override strategy decision: the strategy applies to a given task by setting of a probability for processing of events in this task. Initially this probability, or velocity, is set to 1.0. It can be set to any number between 1% and 100%. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_task_velocity](velocity.h.md#function-lotto_task_velocity) | Sets the task velocity that must be between 1 and 100.  |

##  Function `lotto_task_velocity`

```c
void lotto_task_velocity(int64_t probability) __attribute__((weak))
``` 
_Sets the task velocity that must be between 1 and 100._ 




**Parameters:**

- `probability`: probability 





---
