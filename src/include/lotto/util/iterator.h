/*
 */
#ifndef LOTTO_ITERATOR_H
#define LOTTO_ITERATOR_H

#include <stdint.h>

#include <lotto/sys/memory.h>

typedef struct iter_s {
    void *obj;
    uint64_t pos;
    uint64_t size;
} iter_t;

typedef struct iter_args_s {
    void *arg;
    void *ret;
} iter_args_t;

/*******************************************************************************
 *
 * If the array and size are globals, you can define a shorter and more
 *decriptive macro locally #define ITERATE_PLUGINS(MAP, MAP_ARGS, REDUCE,
 *REDUCE_ARGS, REDUCE_RET) ITERATE_ARRAY(_plugins, plugin_t, _next, MAP,
 *MAP_ARG, REDUCE, REDUCE_ARG, REDUCE_RET)
 *
 * MAP and REDUCE are function pointers with the following signature
 *
 * static void map(TYPE* array_ptr, iter_args_t *map_arg);
 * static void reduce(iter_args_t *map_args, uint64_t size, iter_args_t
 **reduce_arg);
 *
 * For REDUCE() map_args is an array iter_args_t map_arg[SIZE];
 *
 * MAP_ARG, REDUCE_ARG and REDUCE_RET are pointers to arbitrary datastructures
 * that may be used in the MAP or REDUCE functions
 *
 * MAP_ARG is passed as map_arg->arg to MAP()
 * REDUCE_ARG and REDUCE_RET are passed as reduce_arg->arg and reduce_arg->ret
 *
 * Results from MAP() should be stored in TYPE_MAP_RET map_returns[SIZE];
 * Accessible as map_arg->ret
 *
 ******************************************************************************/

#define ITER_ARRAY_INIT(ARRAY_PTR, SIZE)                                       \
    {                                                                          \
        .obj = ARRAY_PTR, .pos = 0, .size = SIZE                               \
    }
#define ITER_ARRAY_CUR(ITER, TYPE)                                             \
    (ITER.pos >= ITER.size ? NULL : &(((TYPE *)(ITER.obj))[ITER.pos]))
#define ITERATE_ARRAY(ARRAY_PTR, TYPE, SIZE, MAP, MAP_ARG, TYPE_MAP_RET,       \
                      REDUCE, REDUCE_ARG, REDUCE_RET)                          \
    {                                                                          \
        iter_args_t map_args[SIZE];                                            \
        TYPE_MAP_RET map_returns[SIZE];                                        \
        iter_args_t reduce_args;                                               \
        for (iter_t iter = ITER_ARRAY_INIT(ARRAY_PTR, SIZE);                   \
             NULL != ITER_ARRAY_CUR(iter, TYPE); iter.pos++) {                 \
            map_args[iter.pos].arg = MAP_ARG;                                  \
            map_args[iter.pos].ret = &map_returns[iter.pos];                   \
            MAP(&(((TYPE *)(iter.obj))[iter.pos]), &map_args[iter.pos]);       \
        }                                                                      \
        reduce_args.arg = REDUCE_ARG;                                          \
        reduce_args.ret = REDUCE_RET;                                          \
        REDUCE(map_args, SIZE, &reduce_args);                                  \
    }

#endif
