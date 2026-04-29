/*******************************************************************************
 * modules
 ******************************************************************************/
#include <stdlib.h>
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
    size_t count;
} modules_ctx_t;

typedef struct {
    char name[128];
    int slot;
    bool has_slot;
    bool has_driver;
    bool has_runtime;
    const char *runtime_type;
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
_collect_scanned_module(module_t *module, void *arg)
{
    modules_ctx_t *ctx  = arg;
    module_info_t *mods = (module_info_t *)(ctx + 1);
    const char *name    = _module_basename(module->name);
    module_info_t *m    = _find_or_add(mods, (size_t *)&ctx->count, name);
    if (m == NULL) {
        return 1;
    }
    if (module->kind & MODULE_KIND_CLI) {
        m->has_driver = true;
    }
    if (module->kind & MODULE_KIND_RUNTIME) {
        m->has_runtime  = true;
        m->runtime_type = "plugin";
    }
    return 0;
}

static void
_collect_metadata(module_info_t mods[], size_t *nmods)
{
    size_t count                            = 0;
    const lotto_module_metadata_t *metadata = lotto_module_metadata_all(&count);
    for (size_t i = 0; i < count; i++) {
        const lotto_module_metadata_t *entry = &metadata[i];
        module_info_t *m = _find_or_add(mods, nmods, entry->name);
        if (m == NULL) {
            return;
        }
        m->slot     = entry->slot;
        m->has_slot = true;
        switch (entry->component) {
            case LOTTO_MODULE_COMPONENT_DRIVER:
                m->has_driver = true;
                break;
            case LOTTO_MODULE_COMPONENT_RUNTIME:
                m->has_runtime  = true;
                m->runtime_type = entry->type;
                break;
            default:
                break;
        }
    }
}

static int
_module_info_cmp(const void *pa, const void *pb)
{
    const module_info_t *a = pa;
    const module_info_t *b = pb;
    if (a->has_slot != b->has_slot) {
        return a->has_slot ? -1 : 1;
    }
    if (a->has_slot && a->slot != b->slot) {
        return a->slot - b->slot;
    }
    return sys_strcmp(a->name, b->name);
}

static const char *
_module_enabled_str(const module_info_t *m)
{
    bool default_enabled = false;
    if (module_runtime_switchable_default_enabled(m->name, &default_enabled)) {
        return default_enabled ? "yes" : "no";
    }
    return "always";
}

static size_t
_max_size(size_t a, size_t b)
{
    return a > b ? a : b;
}

static void
_print_modules_table(module_info_t mods[], size_t nmods)
{
    const char *slot_header    = "Slot";
    const char *module_header  = "Module";
    const char *enabled_header = "Enabled";
    const char *type_header    = "Runtime type";
    size_t slot_w              = sys_strlen(slot_header);
    size_t module_w            = sys_strlen(module_header);
    size_t enabled_w           = sys_strlen(enabled_header);
    size_t type_w              = sys_strlen(type_header);
    char slot_buf[32];

    for (size_t i = 0; i < nmods; i++) {
        const module_info_t *m = &mods[i];
        if (m->has_slot) {
            sys_snprintf(slot_buf, sizeof(slot_buf), "%d", m->slot);
            slot_w = _max_size(slot_w, sys_strlen(slot_buf));
        }
        module_w  = _max_size(module_w, sys_strlen(m->name));
        enabled_w = _max_size(enabled_w, sys_strlen(_module_enabled_str(m)));
        type_w    = _max_size(type_w,
                              sys_strlen(m->has_runtime ? m->runtime_type : "-"));
    }

    sys_fprintf(stdout, "%-*s  %-*s  %-*s  %s\n", (int)slot_w, slot_header,
                (int)module_w, module_header, (int)enabled_w, enabled_header,
                type_header);
    for (size_t i = 0; i < nmods; i++) {
        const module_info_t *m = &mods[i];
        if (m->has_slot) {
            sys_snprintf(slot_buf, sizeof(slot_buf), "%d", m->slot);
        } else {
            slot_buf[0] = '-';
            slot_buf[1] = '\0';
        }
        sys_fprintf(stdout, "%*s  %-*s  %-*s  %s\n", (int)slot_w, slot_buf,
                    (int)module_w, m->name, (int)enabled_w,
                    _module_enabled_str(m),
                    m->has_runtime ? m->runtime_type : "-");
    }
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

    _collect_metadata(state.mods, &state.ctx.count);
    lotto_module_foreach_all(_collect_scanned_module, &state.ctx);
    qsort(state.mods, state.ctx.count, sizeof(state.mods[0]), _module_info_cmp);
    _print_modules_table(state.mods, state.ctx.count);
    sys_fprintf(stdout, "Total modules: %lu\n", (uint64_t)state.ctx.count);
    return 0;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {0};
    subcmd_register(modules, "modules", "", "List available Lotto modules",
                    false, sel, flags_default, SUBCMD_GROUP_TRACE);
})
