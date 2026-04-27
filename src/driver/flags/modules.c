#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/csv.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>

static flag_t FLAG_ENABLE_MODULE;
static flag_t FLAG_DISABLE_MODULE;

_FLAGMGR_CALLBACK(enable_module, { (void)v; })
_FLAGMGR_CALLBACK(disable_module, { (void)v; })
_FLAGMGR_SUBSCRIBE({
    FLAG_ENABLE_MODULE =
        new_flag("ENABLE_MODULE", "", "enable", "MODULE",
                 "enable runtime-switchable module MODULE", flag_sval(""),
                 (str_converter_t){}, _flagmgr_callback_enable_module);
    FLAG_DISABLE_MODULE =
        new_flag("DISABLE_MODULE", "", "disable", "MODULE",
                 "disable runtime-switchable module MODULE", flag_sval(""),
                 (str_converter_t){}, _flagmgr_callback_disable_module);
})
FLAG_GETTER(enable_module, ENABLE_MODULE)
FLAG_GETTER(disable_module, DISABLE_MODULE)

typedef struct {
    const char *name;
    module_enable_f *set_enabled;
    bool default_enabled;
} runtime_switchable_module_t;

#define MAX_RUNTIME_SWITCHABLE_MODULES 64

static runtime_switchable_module_t
    _runtime_switchable_modules[MAX_RUNTIME_SWITCHABLE_MODULES];
static size_t _nruntime_switchable_modules;

static void
_normalize_module_name(const char *src, char *dst, size_t n)
{
    ASSERT(n > 0);
    size_t i = 0;
    for (; src[i] != '\0' && i + 1 < n; i++) {
        char c = src[i];
        if (c == '_') {
            c = '-';
        }
        dst[i] = c;
    }
    dst[i] = '\0';
}

static const runtime_switchable_module_t *
_find_runtime_switchable_module(const char *name)
{
    char normalized[128];
    char candidate[128];
    _normalize_module_name(name, normalized, sizeof(normalized));
    for (size_t i = 0; i < _nruntime_switchable_modules; i++) {
        _normalize_module_name(_runtime_switchable_modules[i].name, candidate,
                               sizeof(candidate));
        if (sys_strcmp(candidate, normalized) == 0) {
            return &_runtime_switchable_modules[i];
        }
    }
    return NULL;
}

void
register_runtime_switchable_module(const char *name,
                                   module_enable_f *set_enabled,
                                   bool default_enabled)
{
    ASSERT(name != NULL);
    ASSERT(set_enabled != NULL);

    const runtime_switchable_module_t *existing =
        _find_runtime_switchable_module(name);
    ASSERT(existing == NULL && "duplicate runtime-switchable module");
    ASSERT(_nruntime_switchable_modules < MAX_RUNTIME_SWITCHABLE_MODULES);

    runtime_switchable_module_t *slot =
        &_runtime_switchable_modules[_nruntime_switchable_modules++];
    slot->name            = name;
    slot->set_enabled     = set_enabled;
    slot->default_enabled = default_enabled;
}

bool
module_is_runtime_switchable(const char *name)
{
    return _find_runtime_switchable_module(name) != NULL;
}

bool
module_runtime_switchable_default_enabled(const char *name,
                                          bool *default_enabled)
{
    const runtime_switchable_module_t *module =
        _find_runtime_switchable_module(name);
    if (module == NULL) {
        return false;
    }
    if (default_enabled != NULL) {
        *default_enabled = module->default_enabled;
    }
    return true;
}

static int
_handle_module_token(const char *tok, void *arg)
{
    struct {
        bool enabled;
        bool apply;
    } *ctx = arg;

    const runtime_switchable_module_t *module =
        _find_runtime_switchable_module(tok);
    if (module == NULL) {
        sys_fprintf(stderr, "error: module '%s' is not runtime-switchable\n",
                    tok);
        return 1;
    }
    if (ctx->apply) {
        module->set_enabled(ctx->enabled);
    }
    return 0;
}

static int
_handle_module_csv(const char *csv, bool enabled, bool apply)
{
    struct {
        bool enabled;
        bool apply;
    } ctx = {.enabled = enabled, .apply = apply};
    return csv_for_each(csv, _handle_module_token, &ctx);
}

int
validate_module_enable_flags(const flags_t *flags)
{
    if (_handle_module_csv(flags_get_sval(flags, flag_enable_module()), true,
                           false) != 0) {
        return 1;
    }
    return _handle_module_csv(flags_get_sval(flags, flag_disable_module()),
                              false, false);
}

int
apply_module_enable_flags(const flags_t *flags)
{
    if (_handle_module_csv(flags_get_sval(flags, flag_enable_module()), true,
                           true) != 0) {
        return 1;
    }
    return _handle_module_csv(flags_get_sval(flags, flag_disable_module()),
                              false, true);
}
