add_library(CesiumTilesetJson "")
configure_cesium_library(CesiumTilesetJson)

cesium_glob_files(CESIUM_TILESET_JSON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumTilesetJson/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumTilesetJson/*.h
)
cesium_glob_files(CESIUM_TILESET_JSON_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_TILESET_JSON_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumTilesetJson
    PROPERTIES
        TEST_SOURCES "${CESIUM_TILESET_JSON_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_TILESET_JSON_TEST_HEADERS}"
)

set_target_properties(CesiumTilesetJson
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_TILESET_JSON_PUBLIC_HEADERS}"
)

target_sources(
    CesiumTilesetJson
    PRIVATE
        ${CESIUM_TILESET_JSON_SOURCES}
        ${CESIUM_TILESET_JSON_HEADERS}
        ${CESIUM_TILESET_JSON_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumTilesetJson
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumTilesetJson
    PUBLIC
        CesiumUtility
)
