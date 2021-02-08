cmake_minimum_required(VERSION 3.15)

project(
    cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/extern/Catch2/contrib")

include(CTest)
enable_testing()

add_subdirectory(extern EXCLUDE_FROM_ALL)

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

    set_property(TARGET ${targetName}
        PROPERTY
            ${targetName}_PRIVATE_SRC 1)

    if (BUILD_SHARED_LIBS)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                CESIUM_SHARED=${BUILD_SHARED_LIBS}
        )
    endif()

    if (NOT ${targetName} MATCHES "cesium-native-tests")
        if (TARGET cesium-native-tests)
            set_property(TARGET cesium-native-tests
                PROPERTY
                    ${targetName}_PRIVATE_SRC 1)

            target_link_libraries(cesium-native-tests PUBLIC ${targetName})

            file(GLOB TEST_SOURCES test/*.cpp test/*.h)
            
            target_sources(
                cesium-native-tests
                PRIVATE
                    ${TEST_SOURCES}
            )
        endif()

        string(TOUPPER ${targetName} capitalizedTargetName)

        target_compile_definitions(
            ${targetName}
            PRIVATE
                ${capitalizedTargetName}_BUILDING
        )   
    endif()
endfunction()

add_executable(cesium-native-tests "")
configure_cesium_library(cesium-native-tests)

target_sources(
    cesium-native-tests
    PRIVATE
        test-main.cpp
)

target_link_libraries(cesium-native-tests PUBLIC Catch2::Catch2)

add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(CesiumGltf)
add_subdirectory(CesiumGltfReader)
add_subdirectory(CesiumAsync)
add_subdirectory(CesiumIonClient)
add_subdirectory(Cesium3DTiles)

include(Catch)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    catch_discover_tests(cesium-native-tests)
endif()

add_subdirectory(doc)
