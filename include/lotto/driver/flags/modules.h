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
                                        module_enable_f *set_enabled);
int validate_module_enable_flags(const flags_t *flags);
int apply_module_enable_flags(const flags_t *flags);

#endif
