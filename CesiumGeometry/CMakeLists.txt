cmake_minimum_required(VERSION 3.15)
project(CesiumGeometry CXX)

include(cesium)

add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

cesium_glob_files(CESIUM_GEOMETRY_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GEOMETRY_HEADERS src/*.h)
cesium_glob_files(CESIUM_GEOMETRY_PUBLIC_HEADERS include/CesiumGeometry/*.h)
cesium_glob_files(CESIUM_GEOMETRY_TEST_SOURCES test/*.cpp)

set_target_properties(CesiumGeometry
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GEOMETRY_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGeometry
    PRIVATE
        ${CESIUM_GEOMETRY_SOURCES}
        ${CESIUM_GEOMETRY_HEADERS}
    PUBLIC
        ${CESIUM_GEOMETRY_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeometry
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

find_package(CesiumUtility REQUIRED)

target_link_libraries(CesiumGeometry PUBLIC
    CesiumUtility::CesiumUtility
)

install(TARGETS CesiumGeometry
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGeometry
)

cesium_tests(
  CesiumGeometry
  "${CESIUM_GEOMETRY_TEST_SOURCES}"
)
