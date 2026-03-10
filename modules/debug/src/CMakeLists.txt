add_driver_module(cmd.c module.c)
target_include_directories(lotto-driver-debug PRIVATE ${CMAKE_BINARY_DIR}/generated
                                                     ${CMAKE_BINARY_DIR}/include)
add_dependencies(lotto-driver-debug blob-debug
                                    blob-command
                                    blob-decorator
                                    blob-filter
                                    blob-iterator
                                    blob-matcher
                                    blob-handler
                                    blob-util)
