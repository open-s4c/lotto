#ifndef LOTTO_CONTEXT_H
#define LOTTO_CONTEXT_H
#include <stdint.h>

#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/task_id.h>

#define CTX_NARGS 4

/**
 * Represents the context of an intercepted call.
 */
typedef struct context {
    category_t cat; ///< Category of interception
    task_id id;     ///< Task ID
    task_id vid;    ///< Virtual task ID (NO_TASK if not available)
    uintptr_t pc;   ///< Program counter at the interception
#if defined(QLOTTO_ENABLED)
    uint64_t icount; ///< Time shifter
    uint32_t pstate; ///< Processor state (EL at bits 2-3)
#endif
    const char *func;      ///< Debugging information
    uintptr_t func_addr;   ///< Address of intercepted call
    arg_t args[CTX_NARGS]; ///< Additional arguments
} context_t;

void context_print(const context_t *ctx);

/*******************************************************************************
 * Context constructor macros
 ******************************************************************************/

#define CTX_LEVELS 0

#define ctx_raw(...) (&(context_t){.args = {0}, __VA_ARGS__})

#define ctx(...)                                                               \
    (&(context_t){.func = "UNKNOWN",                                           \
                  .cat  = CAT_NONE,                                            \
                  .id   = NO_TASK,                                             \
                  .pc   = ((uintptr_t)__builtin_return_address(CTX_LEVELS)),   \
                  .func_addr =                                                 \
                      ((uintptr_t)__builtin_frame_address(CTX_LEVELS)),        \
                  .args = {0},                                                 \
                  __VA_ARGS__})

#define ctx_cat(ctx, c) (ctx->cat = c, ctx)

#define ctx_empty (&(context_t){0})

#endif
