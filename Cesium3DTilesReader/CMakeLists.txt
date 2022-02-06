cmake_minimum_required(VERSION 3.15)
project(Cesium3DTilesReader CXX)

include(cesium)

add_library(Cesium3DTilesReader "")
configure_cesium_library(Cesium3DTilesReader)

cesium_glob_files(CESIUM_3DTILES_READER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_READER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_3DTILES_READER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTilesReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/Cesium3DTilesReader/*.h
)
cesium_glob_files(CESIUM_3DTILES_READER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)

set_target_properties(Cesium3DTilesReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_READER_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTilesReader
    PRIVATE
        ${CESIUM_3DTILES_READER_SOURCES}
        ${CESIUM_3DTILES_READER_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_READER_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTilesReader
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

find_cesium_package(Cesium3DTiles)
find_cesium_package(CesiumJsonReader)
find_package(Microsoft.GSL REQUIRED)

target_link_libraries(Cesium3DTilesReader
    PUBLIC
        Microsoft.GSL::GSL
        Cesium3DTiles::Cesium3DTiles
        CesiumJsonReader::CesiumJsonReader
)

install(TARGETS Cesium3DTilesReader
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTilesReader
)

cesium_tests(
  Cesium3DTilesReader
  "${CESIUM_3DTILES_READER_TEST_SOURCES}"
)

add_library(Cesium3DTilesReader::Cesium3DTilesReader ALIAS Cesium3DTilesReader)
