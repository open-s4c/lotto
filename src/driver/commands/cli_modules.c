/*******************************************************************************
 * modules
 ******************************************************************************/
#include <unistd.h>

#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/driver/subcmd.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/modules.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>

typedef struct {
    uint64_t count;
} modules_ctx_t;

typedef struct {
    char name[128];
    const char *driver_path;
    const char *runtime_path;
    bool shadowed;
} module_info_t;

#define MAX_MODULES 128

static const char *
_module_basename(const char *module_name)
{
    const char *p = module_name;
    if (sys_strncmp(p, "driver-", 7) == 0) {
        return p + 7;
    }
    if (sys_strncmp(p, "runtime-", 8) == 0) {
        return p + 8;
    }
    return p;
}

static module_info_t *
_find_or_add(module_info_t mods[], size_t *nmods, const char *name)
{
    for (size_t i = 0; i < *nmods; i++) {
        if (sys_strcmp(mods[i].name, name) == 0) {
            return &mods[i];
        }
    }
    if (*nmods >= MAX_MODULES) {
        return NULL;
    }
    module_info_t *m = &mods[(*nmods)++];
    *m               = (module_info_t){0};
    sys_snprintf(m->name, sizeof(m->name), "%s", name);
    return m;
}

static int
_collect_module(module_t *module, void *arg)
{
    modules_ctx_t *ctx  = arg;
    module_info_t *mods = (module_info_t *)(ctx + 1);
    const char *name    = _module_basename(module->name);
    module_info_t *m    = _find_or_add(mods, (size_t *)&ctx->count, name);
    if (m == NULL) {
        return 1;
    }
    m->shadowed |= module->shadowed;
    if ((module->kind & MODULE_KIND_CLI) &&
        (!module->shadowed || !m->driver_path)) {
        m->driver_path = module->path;
    }
    if ((module->kind & MODULE_KIND_RUNTIME) &&
        (!module->shadowed || !m->runtime_path)) {
        m->runtime_path = module->path;
    }
    return 0;
}

static bool
_flag_matches_module(const char *flag, const char *module)
{
    char handler[160];
    sys_snprintf(handler, sizeof(handler), "handler-%s", module);
    if (sys_strcmp(flag, handler) == 0) {
        return true;
    }
    if (sys_strncmp(flag, module, sys_strlen(module)) == 0) {
        return true;
    }
    return strstr(flag, module) != NULL;
}

static bool
_print_module_flags(const char *module)
{
    const option_t *opts = flagmgr_options();
    bool any             = false;
    for (int i = 1; opts[i].long_opt != NULL; i++) {
        if (opts[i].long_opt[0] == '\0') {
            continue;
        }
        if (_flag_matches_module(opts[i].long_opt, module)) {
            if (!any) {
                sys_fprintf(stdout, "\tflags:");
            }
            sys_fprintf(stdout, " --%s", opts[i].long_opt);
            any = true;
        }
    }
    if (any) {
        sys_fprintf(stdout, "\n");
    }
    return any;
}

/**
 * list available modules
 */
static int
modules(args_t *args, flags_t *flags)
{
    (void)args;
    (void)flags;
    logger(LOGGER_INFO, STDOUT_FILENO);
    struct {
        modules_ctx_t ctx;
        module_info_t mods[MAX_MODULES];
    } state = {0};

    lotto_module_foreach_all(_collect_module, &state.ctx);

    for (uint64_t i = 0; i < state.ctx.count; i++) {
        module_info_t *m = &state.mods[i];
        sys_fprintf(stdout, "Module '%s'\n", m->name);
        if (m->driver_path) {
            sys_fprintf(stdout, "\tplugin=driver\n");
        }
        if (m->runtime_path) {
            sys_fprintf(stdout, "\tbuiltin=runtime\n");
        }
        bool default_enabled = false;
        if (module_runtime_switchable_default_enabled(m->name,
                                                      &default_enabled)) {
            sys_fprintf(stdout, "\tswitchable default=%s\n",
                        default_enabled ? "on" : "off");
        }
        _print_module_flags(m->name);
    }
    sys_fprintf(stdout, "Total modules: %lu\n", state.ctx.count);
    return 0;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {0};
    subcmd_register(modules, "modules", "", "List available Lotto modules",
                    false, sel, flags_default, SUBCMD_GROUP_TRACE);
})
