add_library(Cesium3DTilesWriter "")

configure_cesium_library(Cesium3DTilesWriter)

cesium_glob_files(CESIUM_3DTILES_WRITER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_PUBLIC_HEADERS include/Cesium3DTiles/*.h)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_HEADERS test/*.h)

set_target_properties(Cesium3DTilesWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_3DTILES_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_WRITER_TEST_HEADERS}"
)

target_sources(
    Cesium3DTilesWriter
    PRIVATE
        ${CESIUM_3DTILES_WRITER_SOURCES}
        ${CESIUM_3DTILES_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_WRITER_PUBLIC_HEADERS}
)

target_compile_definitions(
    Cesium3DTilesWriter
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_DEFINES}
)

target_include_directories(
    Cesium3DTilesWriter
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(Cesium3DTilesWriter
    PUBLIC
        Cesium3DTiles
        CesiumJsonReader
        CesiumJsonWriter
)
