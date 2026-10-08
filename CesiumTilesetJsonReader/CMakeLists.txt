add_library(CesiumTilesetJsonReader "")
configure_cesium_library(CesiumTilesetJsonReader)

cesium_glob_files(CESIUM_TILESET_JSON_READER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_READER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_READER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumTilesetJsonReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumTilesetJsonReader/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_READER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_READER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumTilesetJsonReader
    PROPERTIES
        TEST_SOURCES "${CESIUM_TILESET_JSON_READER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_TILESET_JSON_READER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumTilesetJsonReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_TILESET_JSON_READER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumTilesetJsonReader
    PRIVATE
        ${CESIUM_TILESET_JSON_READER_SOURCES}
        ${CESIUM_TILESET_JSON_READER_HEADERS}
        ${CESIUM_TILESET_JSON_READER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumTilesetJsonReader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumTilesetJsonReader
    PUBLIC
        CesiumAsync
        CesiumJsonReader
        CesiumTilesetJson
)
