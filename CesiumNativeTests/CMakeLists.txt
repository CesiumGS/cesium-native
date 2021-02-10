add_executable(cesium-native-tests "")
configure_cesium_library(cesium-native-tests)

target_include_directories(
    cesium-native-tests
    PRIVATE
        ${CESIUM_3DTILES_PUBLIC_INCLUDE_DIR}
        ${CESIUM_3DTILES_INCLUDE_DIR}
        ${CESIUM_UTILITY_PUBLIC_INCLUDE_DIR}
        ${CESIUM_GEOSPATIAL_PUBLIC_INCLUDE_DIR}
        ${CESIUM_GEOMETRY_PUBLIC_INCLUDE_DIR}
        ${CATCH2_SINGLE_INCLUDE_DIR}
)

target_sources(
    cesium-native-tests
    PRIVATE
        ${CESIUM_3DTILES_TEST_SOURCE}
        ${CESIUM_GEOMETRY_TEST_SOURCE}
        ${CESIUM_GEOSPATIAL_TEST_SOURCE}
        ${CESIUM_GLTF_TEST_SOURCE}
        ${CESIUM_GLTF_READER_TEST_SOURCE}
        ${CESIUM_UTILITY_TEST_SOURCE}
        src/test-main.cpp
)

target_link_libraries(
    cesium-native-tests
    PRIVATE
        CesiumAsync
        Cesium3DTiles
        CesiumGeometry
        CesiumGeospatial
        CesiumGltf
        CesiumGltfReader
        Catch2::Catch2
)
