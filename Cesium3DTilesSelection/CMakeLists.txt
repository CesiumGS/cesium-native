cmake_minimum_required(VERSION 3.15)
project(CesiumGltfWriter CXX)

include(cesium)

add_library(Cesium3DTilesSelection "")

configure_cesium_library(Cesium3DTilesSelection)

cesium_glob_files(CESIUM_3DTILES_SELECTION_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_3DTILES_SELECTION_HEADERS src/*.h)
cesium_glob_files(CESIUM_3DTILES_SELECTION_PUBLIC_HEADERS include/Cesium3DTilesSelection/*.h)
cesium_glob_files(CESIUM_3DTILES_SELECTION_TEST_SOURCES test/*.cpp)

set_target_properties(Cesium3DTilesSelection
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_SELECTION_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTilesSelection
    PRIVATE
        ${CESIUM_3DTILES_SELECTION_SOURCES}
        ${CESIUM_3DTILES_SELECTION_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_SELECTION_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTilesSelection
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

find_package(spdlog REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(uriparser REQUIRED)

find_cesium_package(CesiumAsync)
find_cesium_package(CesiumGeospatial)
find_cesium_package(CesiumGeometry)
find_cesium_package(CesiumGltf)
find_cesium_package(CesiumGltfReader)
find_cesium_package(CesiumUtility)

target_link_libraries(Cesium3DTilesSelection
    PUBLIC
        spdlog::spdlog
        tinyxml2::tinyxml2
        uriparser::uriparser
        CesiumAsync::CesiumAsync
        CesiumGeospatial::CesiumGeospatial
        CesiumGeometry::CesiumGeometry
        CesiumGltf::CesiumGltf
        CesiumGltfReader::CesiumGltfReader
        CesiumUtility::CesiumUtility
)

install(TARGETS Cesium3DTilesSelection
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTilesSelection
)

cesium_tests(
  Cesium3DTilesSelection
  "${CESIUM_3DTILES_SELECTION_TEST_SOURCES}"
)

add_library(Cesium3DTilesSelection::Cesium3DTilesSelection ALIAS Cesium3DTilesSelection)
