cmake_minimum_required(VERSION 3.15)
project(CesiumGltfReader CXX)

include(cesium)

add_library(CesiumGltfReader "")
configure_cesium_library(CesiumGltfReader)

cesium_glob_files(CESIUM_GLTF_READER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_READER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_READER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltfReader/*.h
)
cesium_glob_files(CESIUM_GLTF_READER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)

set_target_properties(CesiumGltfReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_READER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltfReader
    PRIVATE
        ${CESIUM_GLTF_READER_SOURCES}
        ${CESIUM_GLTF_READER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_READER_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltfReader
    SYSTEM PUBLIC
        # Necessary for `draco/draco_features.h`
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

find_package(base64 REQUIRED)
find_package(draco REQUIRED)
find_package(stb REQUIRED)
find_package(CesiumAsync REQUIRED)
find_package(CesiumGltf REQUIRED)
find_package(CesiumJsonReader REQUIRED)
find_package(CesiumUtility REQUIRED)

target_link_libraries(CesiumGltfReader
    PUBLIC
        base64::base64
        draco::draco
        stb::stb
        CesiumAsync::CesiumAsync
        CesiumGltf::CesiumGltf
        CesiumJsonReader::CesiumJsonReader
        CesiumUtility::CesiumUtility
)

install(TARGETS CesiumGltfReader
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltfReader
)

cesium_tests(
  CesiumGltfReader
  "${CESIUM_GLTF_READER_TEST_SOURCES}"
)
