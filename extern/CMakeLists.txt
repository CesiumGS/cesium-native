set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
add_subdirectory(Catch2)

add_subdirectory(GSL)

option(URIPARSER_BUILD_TESTS off)
option(URIPARSER_BUILD_DOCS off)
option(URIPARSER_ENABLE_INSTALL off)
option(URIPARSER_BUILD_TOOLS off)
add_subdirectory(uriparser)

if (NOT TARGET draco)
    add_subdirectory(draco)
endif()

if (NOT TARGET glm)
    add_subdirectory(glm)
endif()

set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
add_subdirectory(tinyxml2)

add_subdirectory(asyncplusplus)

add_subdirectory(spdlog)

if (NOT TARGET sqlite3)
    add_subdirectory(sqlite3)
endif()

add_subdirectory(modp_b64)

add_subdirectory(cpp-httplib)

# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
add_library(csprng CSPRNG/source/csprng.cpp)
target_include_directories(
    csprng
    PUBLIC
        CSPRNG/source
)

# PicoSHA2 doesn't have CMakeLists.txt at all
add_library(PicoSHA2 INTERFACE)
target_include_directories(PicoSHA2 SYSTEM INTERFACE PicoSHA2)

set(CESIUM_NATIVE_SPDLOG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/spdlog/include" CACHE INTERNAL
    "Include directory for spdlog"
)

set(CESIUM_NATIVE_SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/sqlite3/" CACHE INTERNAL
    "Include directory for sqlite3"
)

set(CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/rapidjson/include" CACHE INTERNAL
    "Include directory for rapidjson"
)

set(CESIUM_NATIVE_RAPIDJSON_DEFINES 
    RAPIDJSON_HAS_CXX11_RVALUE_REFS
CACHE INTERNAL
    "Compiler definitions for rapidjson"
)

set(CESIUM_NATIVE_DRACO_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/draco/src/" CACHE INTERNAL
    "Include directory for Draco"
)

set(CESIUM_NATIVE_STB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/stb" CACHE INTERNAL
    "Include directory for STB libraries"
)
