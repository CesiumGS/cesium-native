# Use configure_depends to automatically reconfigure on filesystem
# changes at the expense of computational overhead for CMake to
# determine if new files have been added (-DGLOB_USE_CONFIGURE_DEPENDS).
function(cesium_glob_files out_var_name regexes)
	if (NOT DEFINED GLOB_USE_CONFIGURE_DEPENDS)
		set(GLOB_USE_CONFIGURE_DEPENDS OFF CACHE BOOL
			"Controls if cesium-native targets should use configure_depends or not for globbing their sources"
		)
	endif()

    set(files "")
    foreach(arg ${ARGV})
        list(APPEND regexes_only "${arg}")
    endforeach()

    list(POP_FRONT regexes_only)
    if (GLOB_USE_CONFIGURE_DEPENDS)
        file(GLOB_RECURSE files CONFIGURE_DEPENDS ${regexes_only})
    else()
        file(GLOB files ${regexes_only})
    endif()

    set(${ARGV0} "${files}" PARENT_SCOPE)
endfunction()

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (BUILD_SHARED_LIBS)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                CESIUM_SHARED=${BUILD_SHARED_LIBS}
        )
    endif()

    if (NOT ${targetName} MATCHES "cesium-native-tests")
        string(TOUPPER ${targetName} capitalizedTargetName)
        target_compile_definitions(
            ${targetName}
            PRIVATE
                ${capitalizedTargetName}_BUILDING
        )
    endif()
endfunction()

