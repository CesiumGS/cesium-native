cmake_minimum_required(VERSION 3.15)
project(Cesium3DTilesWriter CXX)

include(cesium)

add_library(Cesium3DTilesWriter "")
configure_cesium_library(Cesium3DTilesWriter)

cesium_glob_files(CESIUM_3DTILES_WRITER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_WRITER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTilesWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_3DTILES_WRITER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTilesWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/Cesium3DTilesWriter/*.h
)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)

set_target_properties(Cesium3DTilesWriter
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_WRITER_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTilesWriter
    PRIVATE
        ${CESIUM_3DTILES_WRITER_SOURCES}
        ${CESIUM_3DTILES_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_WRITER_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTilesWriter
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

find_cesium_package(Cesium3DTiles)
find_cesium_package(CesiumJsonWriter)
find_package(Microsoft.GSL REQUIRED)

target_link_libraries(Cesium3DTilesWriter
    PUBLIC
        Microsoft.GSL::GSL
        Cesium3DTiles::Cesium3DTiles
        CesiumJsonWriter::CesiumJsonWriter
)

install(TARGETS Cesium3DTilesWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTilesWriter
)

cesium_tests(
  Cesium3DTilesWriter
  "${CESIUM_3DTILES_WRITER_TEST_SOURCES}"
)

if (CESIUM_TESTS_ENABLED)
  # The 3D Tiles writer does not require the reader, but the writer's tests do.
  # So add the dependency here.
  find_cesium_package(Cesium3DTilesReader)

  target_link_libraries(
    Cesium3DTilesWriter-tests
    PRIVATE
      Cesium3DTilesReader::Cesium3DTilesReader
  )
endif()

add_library(Cesium3DTilesWriter::Cesium3DTilesWriter ALIAS Cesium3DTilesWriter)
