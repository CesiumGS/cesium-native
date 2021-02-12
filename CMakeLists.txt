cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

# Shared object support is currently NOT working on Windows
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)
    else()
        target_compile_options(${targetName} PRIVATE -fPIC -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
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

# Private Libraries
add_subdirectory(extern EXCLUDE_FROM_ALL)

# Public Targets
add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGltf)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(CesiumGltfReader)
add_subdirectory(CesiumGltfWriter)
add_subdirectory(CesiumAsync)
add_subdirectory(Cesium3DTiles)

# Private Targets
add_subdirectory(CesiumNativeTests)
add_subdirectory(doc)
