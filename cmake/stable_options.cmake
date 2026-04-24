option(LOTTO_LTO "Enable link-time optimization" OFF)

# ##############################################################################
# Rust options
# ##############################################################################
# LOTTO_MODULE_rusty is declared via add_module(rusty DEFAULT ${HAVE_CARGO})
# in modules/CMakeLists.txt. The LOTTO_RUST_LIT_FEATURE is set after modules
# are configured in modules/CMakeLists.txt.

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

# ##############################################################################
# QEMU options
# ##############################################################################
# Pre-declare LOTTO_MODULE_qemu so stable_options can set compile definitions
# before modules/CMakeLists.txt runs. The QEMU module owns its dependency
# checks and is disabled by default.
option(LOTTO_MODULE_qemu "Build Lotto with QEMU (qlotto) support" OFF)

if(LOTTO_MODULE_qemu)
    add_compile_definitions(QLOTTO_ENABLED)
    add_compile_definitions(DEFAULT_SLACK_TIME=0)
endif()

option(LOTTO_EMBED_LIB "Embed libplotto.so in Lotto CLI" OFF)

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


# ##############################################################################
# Logging options
# ##############################################################################

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
