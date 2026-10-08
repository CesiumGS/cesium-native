add_library(CesiumGltfConverters "")
configure_cesium_library(CesiumGltfConverters)

cesium_glob_files(CESIUM_GLTF_CONVERTERS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_CONVERTERS_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_CONVERTERS_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfConverters/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltfConverters/*.h
)
cesium_glob_files(CESIUM_GLTF_CONVERTERS_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_GLTF_CONVERTERS_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumGltfConverters
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_CONVERTERS_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_CONVERTERS_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumGltfConverters
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_CONVERTERS_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltfConverters
    PRIVATE
        ${CESIUM_GLTF_CONVERTERS_SOURCES}
        ${CESIUM_GLTF_CONVERTERS_HEADERS}
        ${CESIUM_GLTF_CONVERTERS_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltfConverters
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumGltfConverters
    PUBLIC
        CesiumAsync
        CesiumGeometry
        CesiumGeospatial
        CesiumGltf
        CesiumGltfContent
        CesiumGltfReader
        CesiumTilesetJson
        CesiumTilesetJsonReader
        CesiumUtility
    PRIVATE
        libmorton::libmorton
)
