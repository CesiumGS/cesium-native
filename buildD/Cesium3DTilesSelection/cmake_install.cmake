# Install script for directory: /home/betto/src/cesium-native-ooc/Cesium3DTilesSelection

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/Cesium3DTilesSelection/libCesium3DTilesSelectiond.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/Cesium3DTilesSelection" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/BingMapsRasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/BoundingVolume.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/CreditSystem.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/DebugColorizeTilesRasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/ErrorList.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/GltfConverterResult.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/GltfConverters.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/GltfUtilities.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/IPrepareRendererResources.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/ITileExcluder.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/IonRasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/Library.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterMappedTo3DTile.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlayCollection.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlayDetails.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlayTile.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterOverlayTileProvider.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/Tile.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileContent.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileID.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileMapServiceRasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileOcclusionRendererProxy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileRefine.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TileSelectionState.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/Tileset.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TilesetContentLoader.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TilesetExternals.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/TilesetOptions.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/ViewState.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/ViewUpdateResult.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/WebMapServiceRasterOverlay.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/registerAllTileContentTypes.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/spdlog-cesium.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTilesSelection/include/Cesium3DTilesSelection/tinyxml-cesium.h"
    )
endif()

