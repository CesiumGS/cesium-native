function(configure_cesium_library targetName)
    if (CESIUM_TRACING_ENABLED)
        target_compile_definitions(${targetName} -DCESIUM_TRACING_ENABLED=1)
    endif()

    # if (MSVC)
    #     target_compile_options(${targetName} PRIVATE /WX)
    # endif()


    target_compile_definitions(
        ${targetName}
        PUBLIC
            GLM_FORCE_XYZW_ONLY # Disable .rgba and .stpq to make it easier to view values from debugger
            GLM_FORCE_EXPLICIT_CTOR # Disallow implicit conversions between dvec3 <-> dvec4, dvec3 <-> fvec3, etc
            GLM_FORCE_SIZE_T_LENGTH # Make vec.length() and vec[idx] use size_t instead of int
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

    if (CESIUM_EXTRA_INCLUDES)
      target_include_directories(${targetName} PRIVATE ${CESIUM_EXTRA_INCLUDES})
    endif()

endfunction()
