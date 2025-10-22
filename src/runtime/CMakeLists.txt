add_library(lotto_runtime SHARED runtime.c)
set_target_properties(lotto_runtime PROPERTIES OUTPUT_NAME lotto)

# Pull in dice object files from the subtree build.
target_sources(lotto_runtime PRIVATE
    $<TARGET_OBJECTS:dice.o>
    $<TARGET_OBJECTS:dice-dispatch.o>
    $<TARGET_OBJECTS:dice-self.o>
)

target_include_directories(lotto_runtime
    PUBLIC
        $<BUILD_INTERFACE:${LOTTO_DICE_INCLUDE_DIR}>
)

target_link_libraries(lotto_runtime
    PUBLIC
        dice
)

install(TARGETS lotto_runtime)
