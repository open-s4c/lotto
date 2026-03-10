add_driver_module(cmd.c module.c)
target_include_directories(lotto-driver-inflex PRIVATE ${CMAKE_BINARY_DIR}/generated
                                                      ${CMAKE_BINARY_DIR}/include)
