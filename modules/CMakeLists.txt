enable_language(CXX)
# disable clang tidy for this folder
unset(CMAKE_C_CLANG_TIDY)

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

add_subdirectory(priority)
