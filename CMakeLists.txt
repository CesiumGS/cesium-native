cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(NOT DEFINED CMAKE_C_COMPILER_LAUNCHER AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(CCACHE_FOUND ccache)
    find_program(SCCACHE_FOUND sccache)
    if (SCCACHE_FOUND)
        message("setting SCCACHE to ${SCCACHE_FOUND}")
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_FOUND})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_FOUND})
    elseif(CCACHE_FOUND)
        message("setting CCACHE to ${CCACHE_FOUND}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
    endif()
endif()

# ccache/sccache only works with /Z7, not /Zi, so tweak the Debug and RelWithDebInfo build flags in the presence of a compiler cache
if(MSVC AND (DEFINED CMAKE_C_COMPILER_LAUNCHER))
    message(DEBUG "Setting MSVC flags to /Z7 for ccache compatibility.  Current flags: ${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    message(DEBUG "New flags: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

option(PRIVATE_CESIUM_SQLITE "ON to rename SQLite symbols to cesium_sqlite3_* so they won't conflict with other SQLite implemenentations" OFF)
option(CESIUM_TRACING_ENABLED "Whether to enable the Cesium performance tracing framework (CESIUM_TRACE_* macros)." OFF)
option(CESIUM_COVERAGE_ENABLED "Whether to enable code coverage" OFF)
option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)
option(CESIUM_GLM_STRICT_ENABLED "Whether to force strict GLM compile definitions." ON)
option(CESIUM_DISABLE_DEFAULT_ELLIPSOID "Whether to disable the WGS84 default value for ellipsoid parameters across cesium-native." OFF)
option(CESIUM_MSVC_STATIC_RUNTIME_ENABLED "Whether to enable static linking for MSVC runtimes" OFF)

if (CESIUM_MSVC_STATIC_RUNTIME_ENABLED)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (CESIUM_TRACING_ENABLED)
    add_compile_definitions(CESIUM_TRACING_ENABLED=1)
endif()

# Add Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/extern/cmake-modules/")

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

set(CESIUM_DEBUG_POSTFIX "d" CACHE STRING "debug postfix for cesium native")
set(CESIUM_RELEASE_POSTFIX "" CACHE STRING "release postfix for cesium native")

set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

# Use configure_depends to automatically reconfigure on filesystem
# changes at the expense of computational overhead for CMake to
# determine if new files have been added (-DGLOB_USE_CONFIGURE_DEPENDS).

function(cesium_glob_files out_var_name regexes)
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

# Workaround for targets that erroneously forget to
# declare their include directories as `SYSTEM`
function(target_link_libraries_system target scope)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)

    if ("${lib_include_dirs}" MATCHES ".*NOTFOUND$")
        message(FATAL_ERROR "${target}: Cannot use INTERFACE_INCLUDE_DIRECTORIES from target ${lib} as it does not define it")
    endif()

    target_include_directories(${target} SYSTEM ${scope} ${lib_include_dirs})
    target_link_libraries(${target} ${scope} ${lib})
  endforeach()
endfunction()

# Shared object support is currently NOT working on Windows
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201 /bigobj /w45038 /w44254 /w44242 /w44191 /w45220)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
        # Disable dangling-reference warning due to amount of false positives: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109642
        target_compile_options(${targetName} PRIVATE -Wno-dangling-reference)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (CESIUM_GLM_STRICT_ENABLED)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                GLM_FORCE_XYZW_ONLY # Disable .rgba and .stpq to make it easier to view values from debugger
                GLM_FORCE_EXPLICIT_CTOR # Disallow implicit conversions between dvec3 <-> dvec4, dvec3 <-> fvec3, etc
                GLM_FORCE_SIZE_T_LENGTH # Make vec.length() and vec[idx] use size_t instead of int
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
    endif()

    if (CESIUM_EXTRA_INCLUDES)
      target_include_directories(${targetName} PRIVATE ${CESIUM_EXTRA_INCLUDES})
    endif()

endfunction()


# Private Libraries
add_subdirectory(extern EXCLUDE_FROM_ALL)

# These libraries override the debug postfix, so re-override it.
set_target_properties(spdlog PROPERTIES DEBUG_POSTFIX "${CESIUM_DEBUG_POSTFIX}")
set_target_properties(tinyxml2 PROPERTIES DEBUG_POSTFIX "${CESIUM_DEBUG_POSTFIX}")

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
add_subdirectory(CesiumQuantizedMeshTerrain)

# Private Targets
if (CESIUM_TESTS_ENABLED)
    # enable_testing() MUST be called before add_subdirectory or no tests
    # will be found by ctest
    enable_testing()
    add_subdirectory(CesiumNativeTests)
endif()

add_subdirectory(doc)

# Installation of third-party libraries required to use cesium-native
install(TARGETS uriparser OPTIONAL PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/uriparser")

install(DIRECTORY extern/glm/glm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT GLM
)

install(TARGETS tinyxml2)

install(TARGETS Async++)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/extern/asyncplusplus/include/async++.h TYPE INCLUDE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/asyncplusplus/include/async++ TYPE INCLUDE)

install(TARGETS spdlog)
install(DIRECTORY ${CESIUM_NATIVE_SPDLOG_INCLUDE_DIR}/spdlog DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${CESIUM_NATIVE_DRACO_LIBRARY})

install(TARGETS sqlite3)

install(TARGETS modp_b64)

install(TARGETS httplib)

# Don't install CSPRNG when building for Universal Windows Platform
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
  install(TARGETS csprng)
endif()

install(TARGETS GSL)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/GSL/include/gsl TYPE INCLUDE)

# Install with framework destination specified (needed for iOS)
install(TARGETS ktx
		FRAMEWORK
			DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS webpdecoder)

install(DIRECTORY $<TARGET_LINKER_FILE:turbojpeg> TYPE LIB)

install(DIRECTORY $<TARGET_LINKER_FILE:zlibstatic> TYPE LIB)

install(DIRECTORY ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}/rapidjson TYPE INCLUDE)

install(TARGETS s2geometry)

install(TARGETS expected-lite)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/expected-lite/include/nonstd TYPE INCLUDE)

install(TARGETS meshoptimizer)
