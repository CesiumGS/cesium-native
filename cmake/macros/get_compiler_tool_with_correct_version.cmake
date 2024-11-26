# Utility function for getting the path of a compiler tool
function(get_compiler_tool_with_correct_version)
    cmake_parse_arguments(
        ""
        ""
        "TOOL_NAME;TOOLCHAIN_NAME;RESULT_TOOL_PATH"
        ""
        ${ARGN})

    if(NOT _TOOL_NAME)
        message(FATAL_ERROR "TOOL_NAME was not specified")
    endif()

    if(NOT _TOOLCHAIN_NAME)
        message(FATAL_ERROR "TOOLCHAIN_NAME was not specified")
    endif()

    if(NOT _RESULT_TOOL_PATH)
        message(FATAL_ERROR "RESULT_TOOL_PATH was not specified")
    endif()

    set(search_name_without_version "${_TOOL_NAME}")

    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "${_TOOLCHAIN_NAME}")
        # Check if this tool is available with the same version as the compiler
        set(semver_regex "^([0-9]+)")
        # cmake-format: off
        string(REGEX MATCH ${semver_regex} _ "${CMAKE_CXX_COMPILER_VERSION}")
        # cmake-format: on
        set(compiler_major_version "${CMAKE_MATCH_1}")
        set(search_name_with_version "${search_name_without_version}-${compiler_major_version}")

        find_program(search_path "${search_name_with_version}" NO_CACHE)
        if(NOT search_path)
            find_program(search_path "${search_name_without_version}" NO_CACHE)
        endif()
    else()
        # The tool isn't part of the active compiler, so just use the
        # default tool name instead of guessing a version number.
        find_program(search_path "${search_name_without_version}" NO_CACHE)
    endif()

    set(${_RESULT_TOOL_PATH}
        "${search_path}"
        PARENT_SCOPE)
endfunction()