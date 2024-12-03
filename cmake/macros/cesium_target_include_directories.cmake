# Custom handling for target_include_directories to deal with the SYSTEM keyword on public headers.
function(cesium_target_include_directories)
    cmake_parse_arguments(
        ""
        ""
        "TARGET"
        "PUBLIC;PRIVATE"
        ${ARGN})

    if(NOT _TARGET)
        message(FATAL_ERROR "TARGET was not specified.")
    endif()

    if(NOT _PUBLIC)
        message(FATAL_ERROR "PUBLIC was not specified.")
    endif()

    # If cesium-native has been included by another CMake project, we want our headers to be marked `SYSTEM` 
    # so they're not causing a bunch of warnings in another project that may have stricter warnings than we do. 
    # But if we're *not* included in another CMake project, we *want* those warnings, so we need to remove `SYSTEM`.
    if(PROJECT_IS_TOP_LEVEL)
        target_include_directories(
            ${_TARGET}
            PUBLIC
                ${_PUBLIC}
            PRIVATE
                ${_PRIVATE}
        )
    else()
        target_include_directories(
            ${_TARGET}
            SYSTEM PUBLIC
                ${_PUBLIC}
            PRIVATE
                ${_PRIVATE}
        )
    endif()
endfunction()