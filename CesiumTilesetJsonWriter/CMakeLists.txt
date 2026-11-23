add_library(CesiumTilesetJsonWriter "")
configure_cesium_library(CesiumTilesetJsonWriter)

cesium_glob_files(CESIUM_TILESET_JSON_WRITER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_WRITER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumTilesetJsonWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_WRITER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumTilesetJsonWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumTilesetJsonWriter/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_WRITER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_WRITER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumTilesetJsonWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_TILESET_JSON_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_TILESET_JSON_WRITER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumTilesetJsonWriter
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_TILESET_JSON_WRITER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumTilesetJsonWriter
    PRIVATE
        ${CESIUM_TILESET_JSON_WRITER_SOURCES}
        ${CESIUM_TILESET_JSON_WRITER_HEADERS}
        ${CESIUM_TILESET_JSON_WRITER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumTilesetJsonWriter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumTilesetJsonWriter
    PUBLIC
        CesiumJsonWriter
        CesiumTilesetJson
)
