# This function requires the caller to call this function only from the list
# directory containing <submodule_name>.
function(v_ensure_updated_submodule submodule_name)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${submodule_name}")
        message(
            FATAL_ERROR
                "Failed to find ${submodule_name} in ${CMAKE_CURRENT_LIST_DIR}")
    endif()
    # Initialize the submodule in case it is not initialized yet
    execute_process(
        COMMAND git submodule status "${submodule_name}"
        OUTPUT_VARIABLE "submodule_status"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        RESULT_VARIABLE submodule_status_result)
    if(submodule_status_result)
        message(
            WARNING
                "Checking submodule status of ${submodule_name} failed with: ${submodule_status_result}"
        )
        return()
    endif()
    if("${submodule_status}" MATCHES "^-")
        message(CHECK_START "Initializing submodule ${submodule_name}")
        execute_process(
            COMMAND git submodule update --init "${submodule_name}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
            RESULT_VARIABLE submodule_init_result)
        if(submodule_init_result)
            message(CHECK_FAIL "failed with:  ${submodule_status_result}")
            message(WARNING "Submodule ${submodule_name} may be uninitialized")
            return()
        else()
            message(CHECK_PASS "done")
        endif()
        # A literal + prefix means the currently checked out commit does not
        # match the expected commit
    elseif("${submodule_status}" MATCHES "^\\+")
        message(CHECK_START "Updating submodule: ${submodule_name}")
        execute_process(
            COMMAND git submodule update ${submodule_name}
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
            RESULT_VARIABLE submodule_update_result)
        if(submodule_update_result)
            message(CHECK_FAIL "Failed: ${submodule_update_result}")
            message(WARNING "Submodule ${submodule_name} may be out of date")
            return()
        else()
            message(CHECK_PASS "done")
        endif()
    elseif("${submodule_status}" MATCHES "^U")
        message(
            FATAL_ERROR
                "Git reported merge conflicts in the submodule ${submodule_name}."
        )
        return()
    endif()
endfunction()

# Adds a package `pkg_name` as a subdirectory, assuming it is a submodule. If
# CPM_<pkg_name>_SOURCE is set, `CPM` is used if available to add the package.
function(v_add_submodule pkg_name)
    if(DEFINED "CPM_${pkg_name}_SOURCE")
        # Use CPM to add the package if CPM is already available, otherwise just
        # respect the CPM variable and use the given directory as the
        # ${pkg_name} source instead of the submodule
        if(COMMAND CPMAddPackage)
            message(DEBUG
                    "Adding ${pkg_name} via CPM from ${CPM_${pkg_name}_SOURCE}")
            CPMAddPackage(NAME "${pkg_name}")
        else()
            message(DEBUG "Adding ${pkg_name} from ${CPM_${pkg_name}_SOURCE}")
            add_subdirectory("${CPM_${pkg_name}_SOURCE}"
                             "${CMAKE_CURRENT_BINARY_DIR}/${pkg_name}")
        endif()
    else()
        v_ensure_updated_submodule("${pkg_name}")
        add_subdirectory("${pkg_name}" EXCLUDE_FROM_ALL)
    endif()
endfunction()
