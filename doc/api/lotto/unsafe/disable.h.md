#  [lotto](../README.md) / [unsafe](README.md) / disable.h
_The interceptor disable interface._ 

The disable interface allows one to prevent Lotto from receiving capture points. Note that in this case the disabled region will be executed atomically. This is an unsafe interface and the user is not recommended to use it. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_disable](disable.h.md#function-lotto_disable) | Disable the interceptor.  |
| [lotto_enable](disable.h.md#function-lotto_enable) | Enable the interceptor.  |
| [lotto_disabled](disable.h.md#function-lotto_disabled) | Returns whether the interceptor is disabled.  |

##  Function `lotto_disable`

```c
static void lotto_disable(void)
``` 
_Disable the interceptor._ 



##  Function `lotto_enable`

```c
static void lotto_enable(void)
``` 
_Enable the interceptor._ 



##  Function `lotto_disabled`

```c
static bool lotto_disabled(void)
``` 
_Returns whether the interceptor is disabled._ 




---
