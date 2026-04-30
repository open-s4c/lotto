set(SLOT 16) # we pick runtime

enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
set(TIKL_MODULE_FEATURES)

include_directories(blocking/include)
include_directories(clock/include)
include_directories(ichpt/include)
include_directories(mutex/include)
include_directories(timeout/include)
include_directories(rusty/include)
include_directories(evec/include)
include_directories(order/include)
include_directories(region_preemption/include)
include_directories(deadlock/include)
include_directories(concurrent/include)
include_directories(ghost/include)
include_directories(time/include)
include_directories(task_velocity/include)
include_directories(yield/include)
include_directories(available/include)
include_directories(bias/include)
include_directories(terminate/include)
include_directories(explore/include)
include_directories(autocept/include)
include_directories(qemu/include)
include_directories(qemu_gdb/include)
include_directories(qemu_profile/include)
include_directories(qemu_snapshot/include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOTTO_MODULE_BUILD_DIR}")

set(RUNTIME_MODULE_TYPE_guard BUILTIN)
set(RUNTIME_MODULE_TYPE_qemu BUILTIN)
set(RUNTIME_MODULE_TYPE_qemu_gdb BUILTIN)
set(RUNTIME_MODULE_TYPE_qemu_profile BUILTIN)
set(RUNTIME_MODULE_TYPE_qemu_snapshot BUILTIN)
set(RUNTIME_MODULE_TYPE_syscall BUILTIN)
set(RUNTIME_MODULE_TYPE_rusty PLUGIN)
set(RUNTIME_MODULE_LTO_rusty OFF)

add_module(guard SLOT 1)
add_module(summary SLOT 2 DEFAULT OFF)

# blocking handlers
add_module(blocking)
add_module(clock)
add_module(join)
add_module(mutex)
add_module(rwlock)
add_module(evec)
add_module(poll)
add_module(timeout)
add_module(cxa)
add_module(impasse)

# hard filters
add_module(region_preemption)
add_module(region_filter)
add_module(sampling SLOT 3)
add_module(filtering)

# feature handlers
add_module(available)
add_module(watchdog)
add_module(busyabort)
add_module(ichpt)
add_module(race)
add_module(memaccess)
add_module(yield)
add_module(terminate)

# advanced handlers
add_module(priority)
add_module(task_velocity)
add_module(order)
add_module(concurrent SLOT 6)
add_module(ghost SLOT 7)
add_module(rusty DEFAULT ${HAVE_CARGO})

# bias and scheduling strategy handlers
add_module(bias)
add_module(pos)
add_module(pct)

# error detectors
add_module(deadlock)
add_module(enforce)
add_module(inactivity)

# other components
add_module(autocept)
add_module(syscall SLOT 5 DEFAULT OFF)
add_module(time)
add_module(random)
add_module(tsan)
add_module(drum DEFAULT ${HAVE_CURSES})
add_module(debug)
add_module(explore)
add_module(stress)
add_module(trace)
add_module(inflex)
add_module(fork)
add_module(malloc)
add_module(user_mempool)
add_module(uafcheck)
add_module(leakcheck)
add_module(qemu DEFAULT OFF)
add_module(qemu_gdb DEFAULT OFF)
add_module(qemu_profile DEFAULT OFF)
add_module(qemu_snapshot DEFAULT OFF)

get_property(LOTTO_MODULE_METADATA GLOBAL PROPERTY LOTTO_MODULE_METADATA)
set(LOTTO_MODULE_METADATA_SRC
    "${CMAKE_CURRENT_BINARY_DIR}/lotto_module_metadata.c")
file(WRITE "${LOTTO_MODULE_METADATA_SRC}"
     "#include <lotto/sys/modules.h>\n\n"
     "static const lotto_module_metadata_t _lotto_module_metadata[] = {\n")
foreach(ENTRY IN LISTS LOTTO_MODULE_METADATA)
    string(REPLACE "|" ";" ENTRY_FIELDS "${ENTRY}")
    list(GET ENTRY_FIELDS 0 SLOT)
    list(GET ENTRY_FIELDS 1 NAME)
    list(GET ENTRY_FIELDS 2 COMPONENT)
    list(GET ENTRY_FIELDS 3 TYPE)
    string(TOUPPER "${COMPONENT}" COMPONENT_UPPER)
    file(
        APPEND "${LOTTO_MODULE_METADATA_SRC}"
        "    {.slot = ${SLOT}, .name = \"${NAME}\", "
        ".component = LOTTO_MODULE_COMPONENT_${COMPONENT_UPPER}, "
        ".type = \"${TYPE}\"},\n")
endforeach()
file(
    APPEND "${LOTTO_MODULE_METADATA_SRC}"
    "};\n\n"
    "const lotto_module_metadata_t *\n"
    "lotto_module_metadata_all(size_t *count)\n"
    "{\n"
    "    if (count != 0) {\n"
    "        *count = sizeof(_lotto_module_metadata) / "
    "sizeof(_lotto_module_metadata[0]);\n"
    "    }\n"
    "    return _lotto_module_metadata;\n"
    "}\n")
target_sources(lotto-driver PRIVATE "${LOTTO_MODULE_METADATA_SRC}")

if(LOTTO_MODULE_rusty)
    set(LOTTO_RUST_LIT_FEATURE RUST_HANDLERS_AVAILABLE)
endif()
set(TIKL_MODULE_FEATURES
    "${TIKL_MODULE_FEATURES}"
    PARENT_SCOPE)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)
