#  [lotto](../README.md) / [unsafe](README.md) / time.h
_The time interface._ 

The time interface allows the user to call the real time function. This function will always return the current time even if lotto is in replay mode, so the returned values will be different during replay. This is an unsafe interface and the user is not recommended to use it. 

---
# Functions 

| Function | Description |
|---|---|
| [real_time](time.h.md#function-real_time) | Returns the real time.  |

##  Function `real_time`

```c
time_t real_time(time_t *tloc) __attribute__((weak))
``` 
_Returns the real time._ 




---
