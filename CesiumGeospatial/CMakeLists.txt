cmake_minimum_required(VERSION 3.15)
project(CesiumGeospatial CXX)

include(cesium)

add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

cesium_glob_files(CESIUM_GEOSPATIAL_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GEOSPATIAL_HEADERS src/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_PUBLIC_HEADERS include/CesiumGeospatial/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_TEST_SOURCES test/*.cpp)

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

find_cesium_package(CesiumUtility)
find_cesium_package(CesiumGeometry)
find_package(earcut REQUIRED)
find_package(s2geometry REQUIRED)

target_link_libraries(CesiumGeospatial PUBLIC
    CesiumGeometry::CesiumGeometry
    CesiumUtility::CesiumUtility
    earcut::earcut
    #PUBLIC
      s2geometry::s2geometry
)

install(TARGETS CesiumGeospatial
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGeospatial
)

cesium_tests(
  CesiumGeospatial
  "${CESIUM_GEOSPATIAL_TEST_SOURCES}"
)

add_library(CesiumGeospatial::CesiumGeospatial ALIAS CesiumGeospatial)
