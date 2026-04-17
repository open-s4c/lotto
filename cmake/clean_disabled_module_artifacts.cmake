if(NOT DEFINED LOTTO_MODULE_BUILD_DIR)
    message(FATAL_ERROR "LOTTO_MODULE_BUILD_DIR is required")
endif()
if(NOT DEFINED SOEXT)
    message(FATAL_ERROR "SOEXT is required")
endif()
if(NOT DEFINED MODULE_NAME AND NOT DEFINED ARTIFACT_PREFIX)
    message(FATAL_ERROR "Either MODULE_NAME or ARTIFACT_PREFIX is required")
endif()

set(_patterns "")
if(DEFINED MODULE_NAME)
    list(
        APPEND
        _patterns
        "${LOTTO_MODULE_BUILD_DIR}/lotto-*${MODULE_NAME}*${SOEXT}"
        "${LOTTO_MODULE_BUILD_DIR}/lotto-*${MODULE_NAME}*.*${SOEXT}"
    )
endif()
if(DEFINED ARTIFACT_PREFIX)
    list(
        APPEND
        _patterns
        "${LOTTO_MODULE_BUILD_DIR}/${ARTIFACT_PREFIX}${SOEXT}"
        "${LOTTO_MODULE_BUILD_DIR}/${ARTIFACT_PREFIX}.*${SOEXT}"
    )
endif()

file(GLOB _candidates ${_patterns})

foreach(_f IN LISTS _candidates)
    file(REMOVE "${_f}")
    message(STATUS "Removed disabled module artifact: ${_f}")
endforeach()
