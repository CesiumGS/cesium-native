add_library(Cesium3DTiles "")
configure_cesium_library(Cesium3DTiles)

set(CESIUM_3DTILES_PUBLIC_HEADERS
    include/Cesium3DTiles/BingMapsRasterOverlay.h
    include/Cesium3DTiles/BoundingVolume.h
    include/Cesium3DTiles/CreditSystem.h
    include/Cesium3DTiles/ExternalTilesetContent.h
    include/Cesium3DTiles/GltfContent.h
    include/Cesium3DTiles/Gltf.h
    include/Cesium3DTiles/IonRasterOverlay.h
    include/Cesium3DTiles/IPrepareRendererResources.h
    include/Cesium3DTiles/Library.h
    include/Cesium3DTiles/RasterMappedTo3DTile.h
    include/Cesium3DTiles/RasterOverlayCollection.h
    include/Cesium3DTiles/RasterOverlayCutoutCollection.h
    include/Cesium3DTiles/RasterOverlay.h
    include/Cesium3DTiles/RasterOverlayTile.h
    include/Cesium3DTiles/RasterOverlayTileProvider.h
    include/Cesium3DTiles/registerAllTileContentTypes.h
    include/Cesium3DTiles/spdlog-cesium.h
    include/Cesium3DTiles/TileContentFactory.h
    include/Cesium3DTiles/TileContentLoadResult.h
    include/Cesium3DTiles/TileContext.h
    include/Cesium3DTiles/Tile.h
    include/Cesium3DTiles/TileID.h
    include/Cesium3DTiles/TileMapServiceRasterOverlay.h
    include/Cesium3DTiles/TileRefine.h
    include/Cesium3DTiles/TileSelectionState.h
    include/Cesium3DTiles/TilesetExternals.h
    include/Cesium3DTiles/Tileset.h
    include/Cesium3DTiles/ViewState.h
    include/Cesium3DTiles/ViewUpdateResult.h
)

set(CESIUM_3DTILES_HEADERS
    src/Batched3DModelContent.h
    src/calcQuadtreeMaxGeometricError.h
    src/CompositeContent.h
    src/JsonHelpers.h
    src/QuantizedMeshContent.h
    src/SkirtMeshMetadata.h
    src/TileUtilities.h
    src/tinyxml2.h
)

set(CESIUM_3DTILES_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" PARENT_SCOPE)
set(CESIUM_3DTILES_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" PARENT_SCOPE)

set(CESIUM_3DTILES_SOURCE
    src/Batched3DModelContent.cpp
    src/BingMapsRasterOverlay.cpp
    src/BoundingVolume.cpp
    src/calcQuadtreeMaxGeometricError.cpp
    src/CompositeContent.cpp
    src/CreditSystem.cpp
    src/ExternalTilesetContent.cpp
    src/Gltf.cpp
    src/GltfContent.cpp
    src/IonRasterOverlay.cpp
    src/JsonHelpers.cpp
    src/QuantizedMeshContent.cpp
    src/RasterMappedTo3DTile.cpp
    src/RasterOverlayCollection.cpp
    src/RasterOverlay.cpp
    src/RasterOverlayCutoutCollection.cpp
    src/RasterOverlayTile.cpp
    src/RasterOverlayTileProvider.cpp
    src/registerAllTileContentTypes.cpp
    src/SkirtMeshMetadata.cpp
    src/TileContentFactory.cpp
    src/Tile.cpp
    src/TileMapServiceRasterOverlay.cpp
    src/Tileset.cpp
    src/TilesetExternals.cpp
    src/TileUtilities.cpp
    src/upsampleGltfForRasterOverlays.cpp
    src/Uri.cpp
    src/ViewState.cpp
)

set(CESIUM_3DTILES_TEST_SOURCE
    test/TestQuantizedMeshContent.cpp
    test/TestSkirtMeshMetadata.cpp
    test/TestUpsampleGltfForRasterOverlay.cpp
)

list(TRANSFORM CESIUM_3DTILES_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_3DTILES_TEST_SOURCE "${CESIUM_3DTILES_TEST_SOURCE}" PARENT_SCOPE)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${CESIUM_3DTILES_SOURCE}
    PUBLIC
        ${CESIUM_3DTILES_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
    PUBLIC
        include
)

target_link_libraries(Cesium3DTiles PUBLIC 
    CesiumAsync 
    CesiumGeospatial 
    CesiumGeometry 
    CesiumGltf 
    CesiumGltfReader 
    CesiumUtility 
    uriparser draco 
    tinyxml2 
    spdlog
)
