cmake_minimum_required(VERSION 3.15)

set(VCPKG_LIBRARY_LINKAGE static)
include(${CMAKE_SOURCE_DIR}/cmake/ezvcpkg/ezvcpkg.cmake)
ezvcpkg_fetch(
    # Current release has a bug in the cmake config for ktx, see https://github.com/microsoft/vcpkg/issues/30787
    COMMIT 10e052511428d6b0c7fcc63a139e8024bb146032
    PACKAGES catch2 gsl ktx picosha2 asyncplusplus draco earcut-hpp expected-lite glm libjpeg-turbo libmorton libwebp meshoptimizer ms-gsl rapidjson s2geometry spdlog sqlite3 stb tinyxml2 uriparser modp-base64 cpp-httplib[core] zlib
    #PACKAGES asyncplusplus
    # Update the cmake toolchain so it can find the above packages
    UPDATE_TOOLCHAIN
    # Force the installation of each package one at a time, or the Travis CI build will time out waiting for output
    SERIALIZE
)


if (NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "Specify the VCPKG toolchain on the command line as '-DCMAKE_TOOLCHAIN_FILE=<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake'")
else()
    message(STATUS "CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Set defaults that should be set BEFORE compiler / IDE detection
include("cmake/defaults.cmake")

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX C
)

option(CESIUM_TRACING_ENABLED "Whether to enable the Cesium performance tracing framework (CESIUM_TRACE_* macros)." OFF)
option(CESIUM_COVERAGE_ENABLED "Whether to enable code coverage" OFF)
option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)
option(CESIUM_GLM_STRICT_ENABLED "Whether to force strict GLM compile definitions." ON)

if (CESIUM_TRACING_ENABLED)
    add_compile_definitions(CESIUM_TRACING_ENABLED=1)
endif()

# Set defaults that need to be set AFTER compiler / IDE detection
include("cmake/compiler.cmake")

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

# On the CI builds, I have to do this explicitly for some reason or it fails to find the vcpkg packages.
# The toolchain is supposed to manage this, but I haven't figured out why it isn't yet.
list(APPEND CMAKE_PREFIX_PATH "${EZVCPKG_DIR}/share/s2")
list(APPEND CMAKE_PREFIX_PATH "${EZVCPKG_DIR}/share")
list(APPEND CMAKE_PREFIX_PATH "${EZVCPKG_DIR}")
# Find the VCPKG dependnecies
# Note that while we could push these into the extern/CMakeLists.txt as an organization tidy-up, that would require
# us to update the minimum version of CMake to 3.24 and to add the GLOBAL option to the find_package calls, otherwise
# they won't be visible in this scope nor any of the subdirectories for the actual libraries.
#
# However, for some of the vcpkg built libraries where they don't provide a prope cmake config file, we have to declare
# and imporeted library target ourselves. This is the case for modp_b64::modp_b64, picosha2::picosha2 and earcut. In
# these cases, we *do* have the somewhat ugly and verbose details in the extern/CMakeLists.txt file.
find_package(Async++ CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)
find_package(expected-lite CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(libmorton CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(s2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(uriparser CONFIG REQUIRED char wchar_t)
find_package(WebP CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# Private Library (s2geometry)
add_subdirectory(extern EXCLUDE_FROM_ALL)

# Public Targets
add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGltf)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(CesiumJsonReader)
add_subdirectory(CesiumJsonWriter)
add_subdirectory(CesiumGltfContent)
add_subdirectory(CesiumGltfReader)
add_subdirectory(CesiumGltfWriter)
add_subdirectory(CesiumAsync)
add_subdirectory(Cesium3DTiles)
add_subdirectory(Cesium3DTilesReader)
add_subdirectory(Cesium3DTilesWriter)
add_subdirectory(Cesium3DTilesContent)
add_subdirectory(CesiumRasterOverlays)
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
