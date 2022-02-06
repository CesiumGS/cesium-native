cmake_minimum_required(VERSION 3.15)
project(Cesium3DTiles CXX)

include(cesium)

add_library(Cesium3DTiles "")
configure_cesium_library(Cesium3DTiles)

cesium_glob_files(CESIUM_3DTILES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_3DTILES_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTiles/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/Cesium3DTiles/*.h
)
cesium_glob_files(CESIUM_3DTILES_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)

set_target_properties(Cesium3DTiles
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${CESIUM_3DTILES_SOURCES}
        ${CESIUM_3DTILES_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

find_package(Microsoft.GSL REQUIRED)
find_cesium_package(CesiumUtility)

target_link_libraries(Cesium3DTiles
    PUBLIC
        Microsoft.GSL::GSL
        CesiumUtility::CesiumUtility
)

install(TARGETS Cesium3DTiles
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTiles
)

cesium_tests(
  Cesium3DTiles
  "${CESIUM_3DTILES_TEST_SOURCES}"
)

add_library(Cesium3DTiles::Cesium3DTiles ALIAS Cesium3DTiles)
