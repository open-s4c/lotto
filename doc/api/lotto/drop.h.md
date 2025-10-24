#  [lotto](README.md) / drop.h
_The drop interface._ 

The SUT can get into situations where a faulty behavior is introduced by the verification process itself. For example, if the SUT is blocked inside a user-defined spin lock, other threads may livelock. The drop interface provides a simple way to simply "drop" the current execution if this happens. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_drop](drop.h.md#function-lotto_drop) | Drop the current execution.  |

##  Function `lotto_drop`

```c
static void lotto_drop()
``` 
_Drop the current execution._ 




---
