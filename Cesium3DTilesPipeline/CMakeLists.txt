add_library(Cesium3DTilesPipeline "")

configure_cesium_library(Cesium3DTilesPipeline)

cesium_glob_files(CESIUM_3DTILES_PIPELINE_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_3DTILES_PIPELINE_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_3DTILES_PIPELINE_PUBLIC_HEADERS include/Cesium3DTilesPipeline/*.h include/Cesium3DTilesPipeline/*.hpp)
cesium_glob_files(CESIUM_3DTILES_PIPELINE_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_3DTILES_PIPELINE_TEST_HEADERS test/*.h test/*.hpp)

set_target_properties(Cesium3DTilesPipeline
    PROPERTIES
        TEST_SOURCES "${CESIUM_3DTILES_PIPELINE_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_PIPELINE_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(Cesium3DTilesPipeline
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_PIPELINE_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTilesPipeline
    PRIVATE
        ${CESIUM_3DTILES_PIPELINE_SOURCES}
        ${CESIUM_3DTILES_PIPELINE_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_PIPELINE_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTilesPipeline
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(Cesium3DTilesPipeline
    PUBLIC
        CesiumAsync
        CesiumGeospatial
        CesiumGeometry
        CesiumGltf
        CesiumGltfReader
        CesiumUtility
        spdlog
    # PRIVATE
        tinyxml2
        uriparser
)

install(TARGETS Cesium3DTilesPipeline
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTilesPipeline
)
