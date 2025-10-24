# ##############################################################################
# Rust options
# ##############################################################################
option(LOTTO_RUST "Enable Lotto Rust modules" OFF)

if("${LOTTO_RUST}")
    set(LOTTO_RUST_LIT_FEATURE RUST_HANDLERS_AVAILABLE)
endif()

# ##############################################################################
# compatibility options
# ##############################################################################
option(LOTTO_EXECINFO "Use execinfo in Lotto runtime" ON)
if(NOT "${LOTTO_EXECINFO}")
    add_definitions(-DDISABLE_EXECINFO)
endif()
option(LOTTO_TSAN_RUNTIME "Use TSAN in Lotto runtime" ON)

option(LOTTO_TESTS_WITH_TSAN "Enable TSAN instrumentation in tests" ON)
if(NOT "${LOTTO_TESTS_WITH_TSAN}")
    add_definitions(-DDISABLE_TSAN_INSTRUMENTATION)
endif()

option(LOTTO_DRUM "Enable Lotto drum" ON)

option(LOTTO_CHIBI "Enable Chibi-Scheme" ON)

# ##############################################################################
# Frontend selection
# ##############################################################################
if(NOT DEFINED LOTTO_FRONTEND_LIST)
    message(FATAL_ERROR "Internal Error: `LOTTO_FRONTEND_LIST` not defined"
                        " before including `stable_options.txt`")
endif()

set(LOTTO_FRONTEND
    "POSIX"
    CACHE STRING "Lotto frontend")
set_property(CACHE LOTTO_FRONTEND PROPERTY STRINGS ${LOTTO_FRONTEND_LIST})

if(NOT LOTTO_FRONTEND IN_LIST LOTTO_FRONTEND_LIST)
    message(FATAL_ERROR "Invalid frontend `${LOTTO_FRONTEND}` selected. "
                        "Please choose from: ${LOTTO_FRONTEND_LIST}")
endif()

if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    add_compile_definitions(QLOTTO_ENABLED)
    add_compile_definitions(DEFAULT_SLACK_TIME=0)
endif()

option(LOTTO_EMBED_LIB "Embed libplotto.so in Lotto CLI" ON)

if(NOT ${LOTTO_EMBED_LIB})
    add_compile_definitions(LOTTO_EMBED_LIB=0)
endif()

# ##############################################################################
# Performance measurements
# ##############################################################################

option(LOTTO_PERF "Enable (q)lotto performance measurements" OFF)

option(LOTTO_PERF_EXAMPLE "Enable (q)lotto performance measurement example" OFF)

if(${LOTTO_PERF_EXAMPLE})
    set(LOTTO_PERF "ON")
    add_compile_definitions(-DLOTTO_PERF_EXAMPLE)
endif()

if(${LOTTO_PERF})
    add_compile_definitions(-DLOTTO_PERF)
endif()

# ##############################################################################
# Runtime options
# ##############################################################################

option(LOTTO_BUSYLOOP_FUTEX "Replace futex with a busyloop in switcher" OFF)

if(${LOTTO_BUSYLOOP_FUTEX})
    add_compile_definitions(FUTEX_USERSPACE)
endif()

if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    set(_H "${_H};capture_group")
    set(_H "${_H};reconstruct")
    set(_H "${_H};inactivity_timeout")
    set(_H "${_H};region_preemption")
    set(_H "${_H};pct")
    set(DEFAULT_DISABLED_HANDLERS "${_H}")
else()
    set(DEFAULT_DISABLED_HANDLERS ";")
endif()

set(LOTTO_DISABLE_HANDLERS
    "${DEFAULT_DISABLED_HANDLERS}"
    CACHE STRING "List of Lotto handlers to be disabled")

option(LOTTO_INTERCEPT_SYSCALL "Intercept syscall()" OFF)
if(${LOTTO_INTERCEPT_SYSCALL})
    add_compile_definitions(LOTTO_INTERCEPT_SYSCALL)
endif()

# ##############################################################################
# Logging options
# ##############################################################################
option(LOTTO_LOGGER_BLOCK "Enable per-file control of logging" OFF)

# ##############################################################################
# Sanitizer options
# ##############################################################################
macro(sanitize)
    if("${LOTTO_ASAN}")
        add_compile_options(-fsanitize=address)
        link_libraries(asan)
    endif()
endmacro()
option(LOTTO_ASAN "Enable address sanitizer for core code" OFF)
