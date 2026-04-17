if(NOT DEFINED LOTTO_MODULE_BUILD_DIR)
    message(FATAL_ERROR "LOTTO_MODULE_BUILD_DIR is required")
endif()
if(NOT DEFINED MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME is required")
endif()
if(NOT DEFINED SOEXT)
    message(FATAL_ERROR "SOEXT is required")
endif()

file(GLOB _candidates
     "${LOTTO_MODULE_BUILD_DIR}/lotto-*${MODULE_NAME}*${SOEXT}"
     "${LOTTO_MODULE_BUILD_DIR}/lotto-*${MODULE_NAME}*.*${SOEXT}")

foreach(_f IN LISTS _candidates)
    file(REMOVE "${_f}")
    message(STATUS "Removed disabled module artifact: ${_f}")
endforeach()
