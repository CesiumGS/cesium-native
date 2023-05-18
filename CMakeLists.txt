cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/cmake/ezvcpkg/ezvcpkg.cmake)
ezvcpkg_fetch(
    COMMIT a7b6122f6b6504d16d96117336a0562693579933
    PACKAGES catch2 gsl ktx picosha2 asyncplusplus draco earcut-hpp expected-lite glm libjpeg-turbo libmorton libwebp rapidjson spdlog stb tinyxml2 uriparser modp-base64 cpp-httplib
    # Update the cmake toolchain so it can find the above packages
    UPDATE_TOOLCHAIN
    # Force the installation of each package one at a time, or the Travis CI build will time out waiting for output
    SERIALIZE
)

if (NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "Specify the VCPKG toolchain on the command line as '-DCMAKE_TOOLCHAIN_FILE=<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake'")
endif()

include("cmake/defaults.cmake")

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX C
)

# Set defaults that need to be set AFTER compiler / IDE detection
include("cmake/compiler.cmake")

option(CESIUM_TRACING_ENABLED "Whether to enable the Cesium performance tracing framework (CESIUM_TRACE_* macros)." OFF)
option(CESIUM_COVERAGE_ENABLED "Whether to enable code coverage" OFF)
option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)

# Add Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/extern/cmake-modules/")
if (CESIUM_COVERAGE_ENABLED AND NOT MSVC)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(
        NAME cesium-native-tests-coverage
        EXECUTABLE ctest -j ${PROCESSOR_COUNT}
        EXCLUDE "${PROJECT_SOURCE_DIR}/extern/*" "${PROJECT_BINARY_DIR}"
        DEPENDENCIES cesium-native-tests
    )
endif()

if (NOT DEFINED GLOB_USE_CONFIGURE_DEPENDS)
    set(GLOB_USE_CONFIGURE_DEPENDS OFF CACHE BOOL
        "Controls if cesium-native targets should use configure_depends or not for globbing their sources"
    )
endif()

# Find the VCPKG dependnecies
find_package(Async++ CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
# find_package(csprng CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)
find_package(expected-lite CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(libmorton CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(uriparser CONFIG REQUIRED char wchar_t)
find_package(WebP CONFIG REQUIRED)

# Additional dependencies
add_subdirectory(extern)

# Public Targets
add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGltf)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(CesiumJsonReader)
add_subdirectory(CesiumJsonWriter)
add_subdirectory(CesiumGltfReader)
add_subdirectory(CesiumGltfWriter)
add_subdirectory(CesiumAsync)
add_subdirectory(Cesium3DTiles)
add_subdirectory(Cesium3DTilesReader)
add_subdirectory(Cesium3DTilesWriter)
add_subdirectory(Cesium3DTilesSelection)
add_subdirectory(CesiumIonClient)

# Private Targets
if (CESIUM_TESTS_ENABLED)
    # enable_testing() MUST be called before add_subdirectory or no tests
    # will be found by ctest
    enable_testing()
    add_subdirectory(CesiumNativeTests)
endif()

add_subdirectory(doc)
