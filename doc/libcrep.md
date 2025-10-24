# libcrep


| Blocking | Deterministic | Calls                   | Optimization (Lotto impl.) |
| -------- | ------------- | ----------------------- | -------------------------- |
|   no     |     yes       | --> real                |          --//--            |
|  yes     |     yes       | lotto --> real          | yield, sleep               |
|  yes     |      no       | lotto --> crep --> real | mutex, condvar, futex      |
|   no     |      no       | crep --> real           | time, random               |

Lotto assumes external calls are deterministic if called in the same order.
libcrep guarantees that they are deterministic if called in the same order.

- Blocking -> we need to record the return *time*
- Nondeterministic -> we need to record the returned *data*
