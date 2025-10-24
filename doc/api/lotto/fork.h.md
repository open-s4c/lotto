#  [lotto](README.md) / fork.h
_The fork interface._ 

Support of different implementations of fork depending on active plugin. 

---
# Functions 

| Function | Description |
|---|---|
| [lotto_fork_execve](fork.h.md#function-lotto_fork_execve) | Wrapper around combination of fork() and execve() system calls.  |

##  Function `lotto_fork_execve`

```c
pid_t lotto_fork_execve(const char *pathname, char *const argv[], char *const envp[]) __attribute__((weak))
``` 
_Wrapper around combination of fork() and execve() system calls._ 


This function returns -1 on error, 0 in a parent process and does not return for a child since it also calls execve(). If execve() itself fails, the child process panics. On fork error, user can observe errno value set by fork(). 



---
