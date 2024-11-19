add_library(CesiumRasterOverlays "")

configure_cesium_library(CesiumRasterOverlays)

cesium_glob_files(CESIUM_RASTER_OVERLAYS_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_RASTER_OVERLAYS_HEADERS src/*.h)
cesium_glob_files(CESIUM_RASTER_OVERLAYS_PUBLIC_HEADERS include/CesiumRasterOverlays/*.h)
cesium_glob_files(CESIUM_RASTER_OVERLAYS_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_RASTER_OVERLAYS_TEST_HEADERS test/*.h)

set_target_properties(CesiumRasterOverlays
    PROPERTIES
        TEST_SOURCES "${CESIUM_RASTER_OVERLAYS_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_RASTER_OVERLAYS_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumRasterOverlays
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_RASTER_OVERLAYS_PUBLIC_HEADERS}"
)

target_sources(
  CesiumRasterOverlays
    PRIVATE
        ${CESIUM_RASTER_OVERLAYS_SOURCES}
        ${CESIUM_RASTER_OVERLAYS_HEADERS}
    PUBLIC
        ${CESIUM_RASTER_OVERLAYS_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumRasterOverlays
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumRasterOverlays
    PUBLIC
        CesiumAsync
        CesiumGeospatial
        CesiumGeometry
        CesiumGltf
        CesiumGltfContent
        CesiumGltfReader
        CesiumUtility
        nonstd::expected-lite
    PRIVATE
        tinyxml2::tinyxml2
)
