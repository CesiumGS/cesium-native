set(VCPKG_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

if (WIN32)
  set(EXTERN_LIB_SUFFIX "lib")
  set(EXTERN_SHARED_LIB_PATH "bin")
  set(EXTERN_SHARED_LIB_SUFFIX "dll")
else()
  set(EXTERN_LIB_SUFFIX "a")
  set(EXTERN_SHARED_LIB_PATH "lib")
  set(EXTERN_SHARED_LIB_SUFFIX "so")
endif()

add_library(include_helpers INTERFACE)
target_include_directories(include_helpers INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")

add_library(earcut INTERFACE)
find_path(EARCUT_HPP_INCLUDE_DIRS "mapbox/earcut.hpp")
target_include_directories(earcut INTERFACE "${EARCUT_HPP_INCLUDE_DIRS}")

# Allow the catch2 cmake include to be found
list(APPEND CMAKE_MODULE_PATH "${VCPKG_ROOT}/share/Catch2")
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

add_library(modp_b64::modp_b64 SHARED IMPORTED GLOBAL)
set_target_properties(modp_b64::modp_b64 PROPERTIES
    INCLUDE_DIRECTORIES "${VCPKG_ROOT}/include"
    IMPORTED_LOCATION "${VCPKG_ROOT}/${EXTERN_SHARED_LIB_PATH}/libmodpbase64.${EXTERN_SHARED_LIB_SUFFIX}"
    IMPORTED_LOCATION_DEBUG "${VCPKG_ROOT}/debug/${EXTERN_SHARED_LIB_PATH}/libmodpbase64.${EXTERN_SHARED_LIB_SUFFIX}"
    IMPORTED_IMPLIB "${VCPKG_ROOT}/lib/libmodpbase64.${EXTERN_LIB_SUFFIX}"
    IMPORTED_IMPLIB_DEBUG "${VCPKG_ROOT}/debug/lib/libmodpbase64.${EXTERN_LIB_SUFFIX}"
)


# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
set(EXTERN_CSPRNG_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/CSPRNG/source")
add_library(csprng STATIC)
target_include_directories(csprng PUBLIC "${EXTERN_CSPRNG_SRC_DIR}")
file(GLOB_RECURSE CSPRNG_HEADERS "${EXTERN_CSPRNG_SRC_DIR}/*.h" "${EXTERN_CSPRNG_SRC_DIR}/*.hpp")
target_sources(csprng PUBLIC ${EXTERN_CSPRNG_SRC_DIR}/csprng.cpp ${CSPRNG_HEADERS})
set_target_properties(csprng PROPERTIES
  FOLDER "External"
)


set(EXTERN_S2_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/s2geometry/src")
# s2geometry's CMake requires OpenSSL, even though it's not needed for any of
# the functionality we actually use. So a simple library with enough functionality
# for our needs is defined here.
add_library(cs_s2geometry STATIC)
target_sources(cs_s2geometry
PRIVATE
  "${EXTERN_S2_SRC_DIR}/s2/s2cell_id.cc"
  "${EXTERN_S2_SRC_DIR}/s2/r2rect.cc"
  "${EXTERN_S2_SRC_DIR}/s2/s2coords.cc"
  "${EXTERN_S2_SRC_DIR}/s2/util/bits/bits.cc"
  "${EXTERN_S2_SRC_DIR}/s2/util/coding/coder.cc"
  "${EXTERN_S2_SRC_DIR}/s2/s2latlng.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/strings/numbers.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/strings/str_cat.cc"
  "${EXTERN_S2_SRC_DIR}/s2/s1angle.cc"
  "${EXTERN_S2_SRC_DIR}/s2/base/stringprintf.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/base/internal/raw_logging.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/strings/internal/memutil.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/strings/ascii.cc"
  "${EXTERN_S2_SRC_DIR}/s2/third_party/absl/base/internal/throw_delegate.cc"
  "${EXTERN_S2_SRC_DIR}/s2/s1interval.cc"
)
set_target_properties(cs_s2geometry PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
  FOLDER "External"
)
target_include_directories(cs_s2geometry PUBLIC "${EXTERN_S2_SRC_DIR}")
if (MSVC)
  target_compile_options(cs_s2geometry PRIVATE "/J" "/wd4127" "/wd4996" "/wd4244" "/wd4267" "/wd4100" "/wd4706")
  target_compile_definitions(cs_s2geometry PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

# FIXME The VCPKG version of s2geometry doesn't work on windows...
# find_package(glog CONFIG REQUIRED)
# find_package(absl CONFIG REQUIRED)
# find_package(unofficial-s2 CONFIG REQUIRED)
# add_library( s2 SHARED IMPORTED )
# set_target_properties(s2 PROPERTIES
#     INCLUDE_DIRECTORIES "${VCPKG_ROOT}/include"
#     IMPORTED_CONFIGURATIONS "Debug;Release"
#     IMPORTED_LOCATION "${VCPKG_ROOT}/lib/s2.lib"
#     IMPORTED_LOCATION_DEBUG "${VCPKG_ROOT}/debug/lib/s2.lib"
# )
