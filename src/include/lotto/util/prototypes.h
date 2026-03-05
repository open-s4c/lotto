#ifndef LOTTO_UTIL_PROTOTYPES_H
#define LOTTO_UTIL_PROTOTYPES_H

/*******************************************************************************
 * argument count macro
 ******************************************************************************/

#define NR_VARS_(m, _00, _01, _02, _03, _04, _05, _06, _07, _08, _09, _10,     \
                 _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22,   \
                 _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, n, ...)     \
    m##n
#define NR_VARS(m, ...)                                                        \
    NR_VARS_(m, ##__VA_ARGS__, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, \
             21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, \
             3, 2, 1, 0)


/*******************************************************************************
 * argument count macro
 ******************************************************************************/

#define NR_VARS_(m, _00, _01, _02, _03, _04, _05, _06, _07, _08, _09, _10,     \
                 _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22,   \
                 _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, n, ...)     \
    m##n
#define NR_VARS(m, ...)                                                        \
    NR_VARS_(m, ##__VA_ARGS__, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, \
             21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, \
             3, 2, 1, 0)


/* *****************************************************************************
 * function prototype parsing and construction
 * ****************************************************************************/

#define RET_TYPE(S)          TYPE_##S
#define FUNC(S)              FUNC_##S
#define VFUNC(S)             VFUNC_##S
#define RL(S)                RL_##S
#define WRAP(S)              WRAP_##S
#define VWRAP(S)             VWRAP_##S
#define _ARGS(S)             ARGS_##S
#define ARGS_TYPEVARS(S)     GET(TYPEVAR, _ARGS(S))
#define ARGS_VARS(S)         GET(VAR, _ARGS(S))
#define TYPE_SIG(T, ...)     T
#define WRAP_SIG(T, F, ...)  sys_##F
#define VWRAP_SIG(T, F, ...) sys_v##F
#define RL_SIG(T, F, ...)    rl_##F
#define FUNC_SIG(T, F, ...)  F
#define VFUNC_SIG(T, F, ...) v##F
#define ARGS_SIG(T, F, ...)  __VA_ARGS__

#define GET(X, ...)              NR_VARS(GET_##X, __VA_ARGS__)(__VA_ARGS__)
#define GET_TYPEVAR12(T, V, ...) T V, GET_TYPEVAR10(__VA_ARGS__)
#define GET_TYPEVAR10(T, V, ...) T V, GET_TYPEVAR8(__VA_ARGS__)
#define GET_TYPEVAR8(T, V, ...)  T V, GET_TYPEVAR6(__VA_ARGS__)
#define GET_TYPEVAR6(T, V, ...)  T V, GET_TYPEVAR4(__VA_ARGS__)
#define GET_TYPEVAR4(T, V, ...)  T V, GET_TYPEVAR2(__VA_ARGS__)
#define GET_TYPEVAR2(T, V)       T V
#define GET_TYPEVAR1(T)          T
#define GET_VAR10(T, V, ...)     V, GET_VAR8(__VA_ARGS__)
#define GET_VAR8(T, V, ...)      V, GET_VAR6(__VA_ARGS__)
#define GET_VAR6(T, V, ...)      V, GET_VAR4(__VA_ARGS__)
#define GET_VAR4(T, V, ...)      V, GET_VAR2(__VA_ARGS__)
#define GET_VAR2(T, V)           V
#define GET_VAR1(T)

#endif // LOTTO_UTIL_PROTOTYPES_H
