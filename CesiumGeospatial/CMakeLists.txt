cmake_minimum_required(VERSION 3.15)
project(CesiumGeospatial CXX)

include(cesium)

add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

cesium_glob_files(CESIUM_GEOSPATIAL_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GEOSPATIAL_HEADERS src/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_PUBLIC_HEADERS include/CesiumGeospatial/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GEOSPATIAL_TEST_HEADERS test/*.h)

set_target_properties(CesiumGeospatial
    PROPERTIES
        TEST_SOURCES "${CESIUM_GEOSPATIAL_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GEOSPATIAL_TEST_HEADERS}"
)

set_target_properties(CesiumGeospatial
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GEOSPATIAL_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGeospatial
    PRIVATE
        ${CESIUM_GEOSPATIAL_SOURCES}
        ${CESIUM_GEOSPATIAL_HEADERS}
    PUBLIC
        ${CESIUM_GEOSPATIAL_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeospatial
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# s2geometry's CMake requires OpenSSL, even though it's not needed for any of
# the functionality we actually use. So a simple library with enough functionality
# for our needs is defined here.
add_library(
  s2geometry
  ../extern/s2geometry/src/s2/s2cell_id.cc
  ../extern/s2geometry/src/s2/r2rect.cc
  ../extern/s2geometry/src/s2/s2coords.cc
  ../extern/s2geometry/src/s2/util/bits/bits.cc
  ../extern/s2geometry/src/s2/util/coding/coder.cc
  ../extern/s2geometry/src/s2/s2latlng.cc
  ../extern/s2geometry/src/s2/third_party/absl/strings/numbers.cc
  ../extern/s2geometry/src/s2/third_party/absl/strings/str_cat.cc
  ../extern/s2geometry/src/s2/s1angle.cc
  ../extern/s2geometry/src/s2/base/stringprintf.cc
  ../extern/s2geometry/src/s2/third_party/absl/base/internal/raw_logging.cc
  ../extern/s2geometry/src/s2/third_party/absl/strings/internal/memutil.cc
  ../extern/s2geometry/src/s2/third_party/absl/strings/ascii.cc
  ../extern/s2geometry/src/s2/third_party/absl/base/internal/throw_delegate.cc
  ../extern/s2geometry/src/s2/s1interval.cc
)
set_target_properties(s2geometry PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
)
target_include_directories(s2geometry SYSTEM PUBLIC ../extern/s2geometry/src)
if (MSVC)
  target_compile_options(s2geometry PRIVATE "/J" "/wd4127" "/wd4996")
  target_compile_definitions(s2geometry PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

find_package(CesiumUtility REQUIRED)
find_package(CesiumGeometry REQUIRED)
find_package(earcut REQUIRED)

target_link_libraries(CesiumGeospatial PUBLIC
    CesiumGeometry::CesiumGeometry
    CesiumUtility::CesiumUtility
    earcut::earcut
    PRIVATE
      s2geometry
)

install(TARGETS CesiumGeospatial
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGeospatial
)
