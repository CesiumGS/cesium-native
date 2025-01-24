function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201 /bigobj /w45038 /w44254 /w44242 /w44191 /w45220)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
        # Disable dangling-reference warning due to amount of false positives: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109642
        target_compile_options(${targetName} PRIVATE -Wno-dangling-reference)
    endif()

    if (CESIUM_GLM_STRICT_ENABLED)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                GLM_FORCE_XYZW_ONLY # Disable .rgba and .stpq to make it easier to view values from debugger
                GLM_FORCE_EXPLICIT_CTOR # Disallow implicit conversions between dvec3 <-> dvec4, dvec3 <-> fvec3, etc
                GLM_ENABLE_EXPERIMENTAL # Allow use of experimental extensions
        )
    endif()

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

        if(CESIUM_INSTALL_HEADERS)
            install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
                DESTINATION ${CMAKE_INSTALL_PREFIX}
            )
            install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated/include
                DESTINATION ${CMAKE_INSTALL_PREFIX}
                OPTIONAL
            )
        endif()

        if(CESIUM_INSTALL_STATIC_LIBS)
            install(TARGETS ${targetName}
                CONFIGURATIONS Release RelWithDebInfo
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
            )
            install(TARGETS ${targetName}
                CONFIGURATIONS Debug
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/debug/lib
            )
        endif()
    endif()

    if (CESIUM_EXTRA_INCLUDES)
      target_include_directories(${targetName} PRIVATE ${CESIUM_EXTRA_INCLUDES})
    endif()
endfunction()
