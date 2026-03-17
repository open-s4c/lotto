sanitize()

include_directories(${CMAKE_BINARY_DIR}/generated)

# The shared driver implementation lives in src/driver. This directory only
# builds the bootstrap executable that loads liblotto-driver.so and forwards
# to driver_main().

add_library(lotto-cli-lib SHARED lib.c)
target_compile_options(lotto-cli-lib PRIVATE -g)
target_include_directories(lotto-cli-lib PUBLIC ${CMAKE_BINARY_DIR}/include)
set_target_properties(lotto-cli-lib PROPERTIES OUTPUT_NAME lotto-cli
                                               LIBRARY_OUTPUT_DIRECTORY
                                               ${CMAKE_BINARY_DIR})

add_executable(lotto-cli main.c)
target_compile_options(lotto-cli PRIVATE -g)
target_link_libraries(lotto-cli PRIVATE lotto-cli-lib base.o sys.o
                                         memmgr_runtime_libc.o m)
target_include_directories(lotto-cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                                             ${CMAKE_BINARY_DIR}/include)
add_dependencies(lotto-cli lotto-driver)
set_target_properties(
    lotto-cli
    PROPERTIES ENABLE_EXPORTS ON
               OUTPUT_NAME lotto
               RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
               INSTALL_RPATH "$ORIGIN/../lib")

install(TARGETS lotto-cli-lib DESTINATION lib)
install(TARGETS lotto-cli DESTINATION bin)
