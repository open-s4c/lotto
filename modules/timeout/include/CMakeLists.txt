add_library(lotto-timeout.h INTERFACE)
target_include_directories(
    lotto-timeout.h
    INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
              "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
