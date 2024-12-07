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
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(Cesium3DTilesWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_3DTILES_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_WRITER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
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

cesium_target_include_directories(
    TARGET
        Cesium3DTilesWriter
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(Cesium3DTilesWriter
    PUBLIC
        Cesium3DTiles
        CesiumJsonWriter
)
