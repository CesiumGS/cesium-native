cmake_minimum_required(VERSION 3.15)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

if (BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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
endfunction()

# Private Libraries
add_subdirectory(extern EXCLUDE_FROM_ALL)

# Public Targets
add_subdirectory(CesiumUtility EXCLUDE_FROM_ALL)
add_subdirectory(CesiumGltf EXCLUDE_FROM_ALL)
add_subdirectory(CesiumGeometry EXCLUDE_FROM_ALL)
add_subdirectory(CesiumGeospatial EXCLUDE_FROM_ALL)
add_subdirectory(CesiumGltfReader EXCLUDE_FROM_ALL)
add_subdirectory(CesiumAsync EXCLUDE_FROM_ALL)
add_subdirectory(Cesium3DTiles EXCLUDE_FROM_ALL)

# Private Targets
add_subdirectory(CesiumNativeTests EXCLUDE_FROM_ALL)
add_subdirectory(doc EXCLUDE_FROM_ALL)
