set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
add_subdirectory(Catch2)

add_subdirectory(GSL)

option(KTX_FEATURE_STATIC_LIBRARY "" on)
option(KTX_FEATURE_TESTS "" off)

# cesium-native uses std::variant features that require iOS 12+. Don't let KTX force
# our build to use iOS 11, because it won't work.
if(IOS)
  set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "12.0" CACHE STRING "iOS Deployment Target")
endif()

add_subdirectory(KTX-Software)

option(WEBP_BUILD_ANIM_UTILS "" off)
option(WEBP_BUILD_CWEBP "" off)
option(WEBP_BUILD_DWEBP "" off)
option(WEBP_BUILD_EXTRAS "" off)
option(WEBP_BUILD_GIF2WEBP "" off)
option(WEBP_BUILD_IMG2WEBP "" off)
option(WEBP_BUILD_LIBWEBPMUX "" off)
option(WEBP_BUILD_VWEBP "" off)
option(WEBP_BUILD_WEBPINFO "" off)
option(WEBP_BUILD_WEBPMUX "" off)
option(WEBP_NEAR_LOSSLESS "" off)
option(WEBP_UNICODE "" off)
add_subdirectory(libwebp)

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

# tinyxml2's CMake build tries to generate a pkg-config file in a way that's incompatible with multi-config generators
# (e.g., Visual Studio). And anyway it's trivial to build manually, so we do that here.
add_library(tinyxml2 tinyxml2/tinyxml2.cpp)
target_include_directories(
    tinyxml2
    PUBLIC
        tinyxml2
)

add_subdirectory(asyncplusplus)

set(SPDLOG_BUILD_TESTING OFF CACHE INTERNAL "Disable SPDLOG Testing")
add_subdirectory(spdlog)

if (NOT TARGET sqlite3)
  add_subdirectory(sqlite3)
endif()

add_subdirectory(modp_b64)

add_subdirectory(expected-lite)

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
set_target_properties(s2geometry PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
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

set(CESIUM_NATIVE_LIBMORTON_INCUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/libmorton/include" CACHE INTERNAL
    "Include directory for libmorton libraries"
)

set(BUILD_TESTING OFF CACHE INTERNAL "Disable libmorton Testing")
add_subdirectory(libmorton)

add_subdirectory(zlib)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    if(NOT IS_ABSOLUTE ${CMAKE_TOOLCHAIN_FILE})
        set(TJ_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/${CMAKE_TOOLCHAIN_FILE})
    else()
        set(TJ_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE})
    endif()
endif()

if(IOS AND XCODE AND NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(TJ_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
else()
    set(TJ_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo)
endif()

include(ExternalProject)
ExternalProject_Add(libjpeg-turbo
    SOURCE_DIR        "${CMAKE_CURRENT_LIST_DIR}/libjpeg-turbo"
    PREFIX            "libjpeg-turbo"
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -B ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo -S ${CMAKE_CURRENT_LIST_DIR}/libjpeg-turbo -DCMAKE_INSTALL_PREFIX=${TJ_INSTALL_PREFIX} -DENABLE_SHARED=0 -DWITH_CRT_DLL=1 -G ${CMAKE_GENERATOR} -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DCMAKE_SYSTEM_PROCESSOR=$<IF:$<BOOL:${CMAKE_SYSTEM_PROCESSOR}>,${CMAKE_SYSTEM_PROCESSOR},unknown> -DCMAKE_BUILD_TYPE=$<CONFIG> -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE} -DCMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI} -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM} -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_DEBUG_POSTFIX=${CMAKE_DEBUG_POSTFIX} -DCMAKE_TOOLCHAIN_FILE=${TJ_TOOLCHAIN_FILE} -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
    BUILD_COMMAND     ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo --config $<CONFIG> --target install
    INSTALL_COMMAND   ""
    BUILD_BYPRODUCTS ${TJ_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX}
                     ${TJ_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
)

add_library(turbojpeg STATIC IMPORTED GLOBAL)
set_property(TARGET turbojpeg PROPERTY IMPORTED_LOCATION ${TJ_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET turbojpeg PROPERTY IMPORTED_LOCATION_DEBUG ${TJ_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX})
target_include_directories(turbojpeg INTERFACE "${CMAKE_CURRENT_LIST_DIR}/libjpeg-turbo")
add_dependencies(turbojpeg libjpeg-turbo)
