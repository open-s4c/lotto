#  [lotto](README.md) / log.h
_The trace reconstruction interface._ 

Log points are used to match the execution against a log sequence. Logs have ids and optional data which are observed after the log is executed. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_log](log.h.md#function-lotto_log) | Inserts logging point.  |
| [lotto_log_data](log.h.md#function-lotto_log_data) | Inserts logging point with data.  |

##  Function `lotto_log`

```c
void lotto_log(const uint64_t id) __attribute__((weak))
``` 
_Inserts logging point._ 




**Parameters:**

- `id`: id 




##  Function `lotto_log_data`

```c
void lotto_log_data(const uint64_t id, int64_t data) __attribute__((weak))
``` 
_Inserts logging point with data._ 




**Parameters:**

- `id`: id 
- `data`: data 





---
