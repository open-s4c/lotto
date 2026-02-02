add_library(lotto-mutex.h INTERFACE)
target_include_directories(
    lotto-mutex.h INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
