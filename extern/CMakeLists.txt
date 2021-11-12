set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
add_subdirectory(Catch2)

add_subdirectory(GSL)

option(URIPARSER_BUILD_TESTS "" off)
option(URIPARSER_BUILD_DOCS "" off)
option(URIPARSER_ENABLE_INSTALL "" on)
option(URIPARSER_BUILD_TOOLS "" off)
add_subdirectory(uriparser)

if ((NOT TARGET draco) AND (NOT TARGET draco_static))
    add_subdirectory(draco)
    if (MSVC)
      set(CESIUM_NATIVE_DRACO_LIBRARY draco)
    else()
      set(CESIUM_NATIVE_DRACO_LIBRARY draco_static)
    endif()
endif()

set(CESIUM_NATIVE_DRACO_LIBRARY ${CESIUM_NATIVE_DRACO_LIBRARY} PARENT_SCOPE)

if (NOT TARGET glm)
    add_subdirectory(glm GLM)
endif()

set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
add_subdirectory(tinyxml2)

add_subdirectory(asyncplusplus)

set(SPDLOG_BUILD_TESTING OFF CACHE INTERNAL "Disable SPDLOG Testing")
add_subdirectory(spdlog)

if (NOT TARGET sqlite3)
  add_subdirectory(sqlite3)
endif()

add_subdirectory(modp_b64)

# s2geometry's CMake requires OpenSSL, even though it's not needed for any of
# the functionality we actually use. So a simple library with enough functionality
# for our needs is defined here.
add_library(
  s2geometry
  s2geometry/src/s2/s2cell_id.cc
  s2geometry/src/s2/r2rect.cc
  s2geometry/src/s2/s2coords.cc
  s2geometry/src/s2/util/bits/bits.cc
  s2geometry/src/s2/util/coding/coder.cc
  s2geometry/src/s2/s2latlng.cc
  s2geometry/src/s2/third_party/absl/strings/numbers.cc
  s2geometry/src/s2/third_party/absl/strings/str_cat.cc
  s2geometry/src/s2/s1angle.cc
  s2geometry/src/s2/base/stringprintf.cc
  s2geometry/src/s2/third_party/absl/base/internal/raw_logging.cc
  s2geometry/src/s2/third_party/absl/strings/internal/memutil.cc
  s2geometry/src/s2/third_party/absl/strings/ascii.cc
  s2geometry/src/s2/third_party/absl/base/internal/throw_delegate.cc
  s2geometry/src/s2/s1interval.cc
)
target_include_directories(s2geometry SYSTEM PUBLIC s2geometry/src)
if (MSVC)
  target_compile_options(s2geometry PRIVATE "/J" "/wd4127" "/wd4996")
  target_compile_definitions(s2geometry PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "Don't use Brotli")
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "Don't use Zlib")
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "Don't use OpenSSL")
add_subdirectory(cpp-httplib)

# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
add_library(csprng CSPRNG/source/csprng.cpp)
target_include_directories(
    csprng
    PUBLIC
        CSPRNG/source
)
set(CSPRNG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.h ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.hpp ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/is_iterable.hpp)

# PicoSHA2 doesn't have CMakeLists.txt at all
add_library(PicoSHA2 INTERFACE)
target_include_directories(PicoSHA2 SYSTEM INTERFACE PicoSHA2)
set_target_properties(PicoSHA2 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/PicoSHA2/picosha2.h")

# Earcut has an old school CMake setup as well with lots of global settings, and it's header only anyway.
# So define a simple library for it here.
add_library(earcut INTERFACE)
target_include_directories(earcut SYSTEM INTERFACE earcut/include)
set_target_properties(earcut PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/earcut/include/mapbox/earcut.hpp")

set(CESIUM_NATIVE_SPDLOG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/spdlog/include" CACHE INTERNAL
    "Include directory for spdlog"
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
