/*
 */
#ifndef LOTTO_QEMU_CALLBACKS_H
#define LOTTO_QEMU_CALLBACKS_H

#include <stdint.h>

#include <lotto/base/task_id.h>

#define REGISTER_GLOBAL_EXTERN(name, ret, ...)                                 \
    extern ret (*name##_f)(__VA_ARGS__);

#define REGISTER_GLOBAL(name, ret, ...) ret (*name##_f)(__VA_ARGS__) = NULL;

#define REGISTER_DECLARE(name, ret, ...)                                       \
    ret name(__VA_ARGS__);                                                     \
    void register_##name(ret (*func)(__VA_ARGS__));

#define REGISTER_DEFINE(name, ret, ...)                                        \
    void register_##name(ret (*func)(__VA_ARGS__))                             \
    {                                                                          \
        name##_f = func;                                                       \
    }

#define REGISTER_DECLARE_GLOBAL_EXTERN(name, ret, ...)                         \
    REGISTER_GLOBAL_EXTERN(name, ret, __VA_ARGS__)                             \
    REGISTER_DECLARE(name, ret, __VA_ARGS__)

#define REGISTER_GLOBAL_DEFINE(name, ret, ...)                                 \
    REGISTER_GLOBAL(name, ret, __VA_ARGS__)                                    \
    REGISTER_DEFINE(name, ret, __VA_ARGS__)

#define REGISTER_GLOBAL_DECLARE_DEFINE(name, ret, ...)                         \
    REGISTER_GLOBAL(name, ret, __VA_ARGS__)                                    \
    REGISTER_DECLARE(name, ret, __VA_ARGS__)                                   \
    REGISTER_DEFINE(name, ret, __VA_ARGS__)

#define WEAK_CALL(name, ...)                                                   \
    if (NULL != name##_f) {                                                    \
        name##_f(__VA_ARGS__);                                                 \
    }

#define REGISTER_CALL(name) register_##name(name);

REGISTER_DECLARE_GLOBAL_EXTERN(qlotto_register_cpu, void,
                               unsigned int cpu_index, void *cpu, void *cpuenv,
                               void *cpustatecc)
REGISTER_DECLARE_GLOBAL_EXTERN(gdb_register_cpu, void, unsigned int cpu_index,
                               void *cpu, void *cpuenv, void *cpustatecc)
REGISTER_DECLARE_GLOBAL_EXTERN(into_qemu_from_guest, void,
                               unsigned int cpu_index)
REGISTER_DECLARE_GLOBAL_EXTERN(into_qemu_from_guest_unsafe, void,
                               unsigned int cpu_index)
REGISTER_DECLARE_GLOBAL_EXTERN(into_guest_from_qemu, void,
                               unsigned int cpu_index)

REGISTER_DECLARE_GLOBAL_EXTERN(in_guest, bool, task_id tid)
REGISTER_DECLARE_GLOBAL_EXTERN(into_lotto_from_qemu, void, task_id tid)
REGISTER_DECLARE_GLOBAL_EXTERN(into_lotto_from_guest, void, task_id tid)
REGISTER_DECLARE_GLOBAL_EXTERN(into_qemu_from_lotto, void, task_id tid)
REGISTER_DECLARE_GLOBAL_EXTERN(into_guest_from_lotto, void, task_id tid)

#endif // LOTTO_QEMU_CALLBACKS_H
