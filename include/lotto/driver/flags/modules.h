/**
 * @file modules.h
 * @brief Driver declarations for generic module enablement flags.
 */
#ifndef LOTTO_DRIVER_FLAGS_MODULES_H
#define LOTTO_DRIVER_FLAGS_MODULES_H

#include <lotto/base/flags.h>

typedef void(module_enable_f)(bool enabled);

flag_t flag_enable_module();
flag_t flag_disable_module();

void register_runtime_switchable_module(const char *name,
                                        module_enable_f *set_enabled,
                                        bool default_enabled);
bool module_is_runtime_switchable(const char *name);
bool module_runtime_switchable_default_enabled(const char *name,
                                               bool *default_enabled);
int validate_module_enable_flags(const flags_t *flags);
int apply_module_enable_flags(const flags_t *flags);

#define REGISTER_RUNTIME_SWITCHABLE_CONFIG(CONFIG_GETTER, DEFAULT_ENABLED)     \
    static void _runtime_switchable_set_enabled(bool enabled)                  \
    {                                                                          \
        (CONFIG_GETTER)->enabled = enabled;                                    \
    }                                                                          \
    _FLAGMGR_SUBSCRIBE({                                                       \
        register_runtime_switchable_module(LOTTO_MODULE_NAME,                  \
                                           _runtime_switchable_set_enabled,    \
                                           (DEFAULT_ENABLED));                 \
    })

#endif
