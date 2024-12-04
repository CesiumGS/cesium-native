add_library(CesiumJsonReader "")
configure_cesium_library(CesiumJsonReader)

cesium_glob_files(CESIUM_JSON_READER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_JSON_READER_HEADERS src/*.h)
cesium_glob_files(CESIUM_JSON_READER_PUBLIC_HEADERS include/CesiumJsonReader/*.h)
cesium_glob_files(CESIUM_JSON_READER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_JSON_READER_TEST_HEADERS test/*.h)

set_target_properties(CesiumJsonReader
    PROPERTIES
        TEST_SOURCES "${CESIUM_JSON_READER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_JSON_READER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumJsonReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_JSON_READER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumJsonReader
    PRIVATE
        ${CESIUM_JSON_READER_SOURCES}
        ${CESIUM_JSON_READER_HEADERS}
    PUBLIC
        ${CESIUM_JSON_READER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumJsonReader
    PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumJsonReader
    PUBLIC
        CesiumUtility
)
