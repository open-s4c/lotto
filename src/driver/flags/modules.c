#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/csv.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static flag_t FLAG_ENABLE_MODULE;
static flag_t FLAG_DISABLE_MODULE;

_FLAGMGR_CALLBACK(enable_module, { (void)v; })
_FLAGMGR_CALLBACK(disable_module, { (void)v; })
_FLAGMGR_SUBSCRIBE({
    FLAG_ENABLE_MODULE =
        new_flag("ENABLE_MODULE", "e", "enable", "MODULE",
                 "enable runtime-switchable module MODULE", flag_sval(""),
                 (str_converter_t){}, _flagmgr_callback_enable_module);
    FLAG_DISABLE_MODULE =
        new_flag("DISABLE_MODULE", "d", "disable", "MODULE",
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

static bool
_module_name_matches(const char *a, const char *b)
{
    char normalized_a[128];
    char normalized_b[128];
    _normalize_module_name(a, normalized_a, sizeof(normalized_a));
    _normalize_module_name(b, normalized_b, sizeof(normalized_b));
    return sys_strcmp(normalized_a, normalized_b) == 0;
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

typedef struct {
    const runtime_switchable_module_t *module;
    bool found;
} module_token_match_t;

static int
_match_module_token(const char *tok, void *arg)
{
    module_token_match_t *match = arg;
    const runtime_switchable_module_t *found =
        _find_runtime_switchable_module(tok);
    if (found == match->module) {
        match->found = true;
    }
    return 0;
}

static bool
_module_csv_contains(const char *csv, const runtime_switchable_module_t *module)
{
    module_token_match_t match = {.module = module};
    csv_for_each(csv, _match_module_token, &match);
    return match.found;
}

typedef struct {
    const char *module;
    char *csv;
    size_t len;
} module_csv_filter_t;

static void
_csv_append(char **csv, size_t *len, const char *tok)
{
    if (tok == NULL || tok[0] == '\0') {
        return;
    }

    size_t tok_len = sys_strlen(tok);
    size_t sep_len = *len == 0 ? 0 : 1;
    char *next     = sys_malloc(*len + sep_len + tok_len + 1);
    ASSERT(next != NULL);
    if (*len != 0) {
        sys_memcpy(next, *csv, *len);
        next[*len] = ',';
    }
    sys_memcpy(next + *len + sep_len, tok, tok_len + 1);
    sys_free(*csv);
    *csv = next;
    *len += sep_len + tok_len;
}

static int
_remove_module_token(const char *tok, void *arg)
{
    module_csv_filter_t *ctx = arg;
    if (_module_name_matches(tok, ctx->module)) {
        return 0;
    }
    _csv_append(&ctx->csv, &ctx->len, tok);
    return 0;
}

static void
_remove_module_from_flag(flags_t *flags, flag_t f, const char *module)
{
    module_csv_filter_t ctx = {.module = module};
    csv_for_each(flags_get_sval(flags, f), _remove_module_token, &ctx);
    flags_set_by_opt(flags, f, sval(ctx.csv == NULL ? "" : ctx.csv));
}

static void
_append_module_to_flag(flags_t *flags, flag_t f, const char *module)
{
    const char *cur = flags_get_sval(flags, f);
    char *csv       = NULL;
    size_t len      = 0;
    _csv_append(&csv, &len, cur);
    _csv_append(&csv, &len, module);
    flags_set_by_opt(flags, f, sval(csv == NULL ? "" : csv));
}

static void
_set_module_flag(flags_t *flags, const char *module, bool enabled)
{
    flag_t enable_flag  = flag_enable_module();
    flag_t disable_flag = flag_disable_module();
    _remove_module_from_flag(flags, enable_flag, module);
    _remove_module_from_flag(flags, disable_flag, module);
    _append_module_to_flag(flags, enabled ? enable_flag : disable_flag, module);
}

typedef struct {
    flags_t *flags;
    bool enabled;
} module_flag_update_t;

static int
_update_module_token(const char *tok, void *arg)
{
    module_flag_update_t *ctx = arg;
    if (!ctx->enabled && _module_name_matches(tok, "all")) {
        for (size_t i = 0; i < _nruntime_switchable_modules; i++) {
            _set_module_flag(ctx->flags, _runtime_switchable_modules[i].name,
                             false);
        }
        return 0;
    }

    const runtime_switchable_module_t *module =
        _find_runtime_switchable_module(tok);
    if (module == NULL) {
        sys_fprintf(stderr, "error: module '%s' is not runtime-switchable\n",
                    tok);
        return 1;
    }
    _set_module_flag(ctx->flags, module->name, ctx->enabled);
    return 0;
}

int
module_enable_flags_update(flags_t *flags, flag_t f, const char *arg)
{
    module_flag_update_t ctx = {
        .flags   = flags,
        .enabled = f == flag_enable_module(),
    };
    return csv_for_each(arg, _update_module_token, &ctx);
}

bool
module_runtime_switchable_enabled(const char *name, const flags_t *flags,
                                  bool *enabled)
{
    const runtime_switchable_module_t *module =
        _find_runtime_switchable_module(name);
    if (module == NULL) {
        return false;
    }

    bool value = module->default_enabled;
    if (flags != NULL) {
        if (_module_csv_contains(flags_get_sval(flags, flag_enable_module()),
                                 module)) {
            value = true;
        }
        if (_module_csv_contains(flags_get_sval(flags, flag_disable_module()),
                                 module)) {
            value = false;
        }
    }
    if (enabled != NULL) {
        *enabled = value;
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
        if (!ctx->enabled && _module_name_matches(tok, "all")) {
            return 0;
        }
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
