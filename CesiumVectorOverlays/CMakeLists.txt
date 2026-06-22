add_library(CesiumVectorOverlays "")

configure_cesium_library(CesiumVectorOverlays)

cesium_glob_files(CESIUM_VECTOR_OVERLAYS_SOURCES 
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
cesium_glob_files(CESIUM_VECTOR_OVERLAYS_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
cesium_glob_files(CESIUM_VECTOR_OVERLAYS_PUBLIC_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/include/CesiumVectorOverlays/*.h)
cesium_glob_files(CESIUM_VECTOR_OVERLAYS_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_VECTOR_OVERLAYS_TEST_HEADERS test/*.h)

set_target_properties(CesiumVectorOverlays
    PROPERTIES
        TEST_SOURCES "${CESIUM_VECTOR_OVERLAYS_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_VECTOR_OVERLAYS_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumVectorOverlays
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_VECTOR_OVERLAYS_PUBLIC_HEADERS}"
)

target_sources(
    CesiumVectorOverlays
    PRIVATE
        ${CESIUM_VECTOR_OVERLAYS_SOURCES}
        ${CESIUM_VECTOR_OVERLAYS_HEADERS}
        ${CESIUM_VECTOR_OVERLAYS_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumVectorOverlays
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumVectorOverlays
    PUBLIC
        CesiumUtility
        CesiumGeometry
        CesiumGeospatial
        CesiumAsync
        CesiumGltf
        CesiumGltfContent
        CesiumImage
        CesiumVectorData
        Cesium3DTilesSelection
        CesiumRasterOverlays
)
