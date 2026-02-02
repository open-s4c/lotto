add_library(lotto-priority.o OBJECT module.c handler.c state.c interceptor.c
                                    category.c)
target_link_libraries(lotto-priority.o PRIVATE lotto.h lotto-src.h dice.o)

add_library(lotto-priority SHARED)
target_link_libraries(lotto-priority PRIVATE lotto-priority.o)
# set_target_properties( lotto_priority_engine PROPERTIES
# LIBRARY_OUTPUT_DIRECTORY
# "${LOTTO_PLUGIN_BUILD_DIR}")

# add_library(lotto_priority_cli SHARED module.c flag.c state.c category.c)
# set_target_properties(lotto_priority_cli PROPERTIES LIBRARY_OUTPUT_DIRECTORY
# "${LOTTO_PLUGIN_BUILD_DIR}")
# target_link_libraries(lotto_priority_cli PRIVATE lotto.h lotto-src.h)
#
# install( TARGETS lotto_priority_engine lotto_priority_cli DESTINATION
# DESTINATION
# "${LOTTO_PLUGIN_INSTALL_DIR}")
