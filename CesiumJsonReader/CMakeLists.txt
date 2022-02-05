cmake_minimum_required(VERSION 3.15)
project(CesiumJsonReader CXX)

include(cesium)

add_library(CesiumJsonReader "")
configure_cesium_library(CesiumJsonReader)

cesium_glob_files(CESIUM_JSON_READER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_JSON_READER_HEADERS src/*.h)
cesium_glob_files(CESIUM_JSON_READER_PUBLIC_HEADERS include/CesiumJsonReader/*.h)
cesium_glob_files(CESIUM_JSON_READER_TEST_SOURCES test/*.cpp)

set_target_properties(CesiumJsonReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_JSON_READER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumJsonReader
    PRIVATE
        ${CESIUM_JSON_READER_SOURCES}
        ${CESIUM_JSON_READER_HEADERS}
    PUBLIC
        ${CESIUM_JSON_READER_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumJsonReader
    SYSTEM PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

find_package(Microsoft.GSL REQUIRED)
find_package(CesiumUtility REQUIRED)

target_link_libraries(CesiumJsonReader
    PUBLIC
      Microsoft.GSL::GSL
      CesiumUtility::CesiumUtility
)

install(TARGETS CesiumJsonReader
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumJsonReader
)

cesium_tests(
  CesiumJsonReader
  "${CESIUM_JSON_READER_TEST_SOURCES}"
)
