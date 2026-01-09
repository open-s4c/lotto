cmake_minimum_required(VERSION 3.16)
project(lotto LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Lotto configuration
# ------------------------------------------------------------------------------

set(LOTTO_LOG_LEVEL
    "INFO"
    CACHE STRING "" FORCE)

# ------------------------------------------------------------------------------
# Dice
# ------------------------------------------------------------------------------
set(DICE_LOG_LEVEL
    "INFO"
    CACHE STRING "" FORCE)

set(DICE_DISPATCH_CHAINS
    "1,4,7,2,3,5,6,0"
    CACHE STRING "" FORCE)

add_subdirectory(deps/dice dice)
include_directories(deps/dice/include)
include_directories(deps/dice/deps/libvsync/include)

# ------------------------------------------------------------------------------
# Lotto source
# ------------------------------------------------------------------------------
include_directories(include)
include_directories(src/include)

include(cmake/utils.cmake)
include(cmake/pubsub.cmake)
add_subdirectory(src)

# ------------------------------------------------------------------------------
# Lotto tests
# ------------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)
