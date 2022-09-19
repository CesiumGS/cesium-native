# Install script for directory: /home/betto/src/cesium-native-ooc/Cesium3DTiles

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/Cesium3DTiles/libCesium3DTilesd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/Cesium3DTiles" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Asset.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Availability.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/BoundingVolume.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Buffer.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/BufferView.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Class.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ClassProperty.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ClassStatistics.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Content.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Enum.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/EnumValue.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesContentGltfLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingAvailabilityLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingBufferLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingBufferViewLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingSubtreeLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesImplicitTilingSubtreesLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataClassLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataClassPropertyLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataClassStatisticsLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataEnumLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataEnumValueLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataGroupMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataPropertyStatisticsLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataSchemaLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataStatisticsLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataSubtreePropertyLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMetadataTilesetMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Extension3dTilesMultipleContentsLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ExtensionContent3dTilesMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ExtensionSubtree3dTilesMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ExtensionSubtree3dTilesMultipleContentsLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ExtensionTile3dTilesMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ExtensionTileset3dTilesMetadataLegacy.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/GroupMetadata.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/ImplicitTiling.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/MetadataEntity.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Properties.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/PropertyStatistics.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/PropertyTable.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/PropertyTableProperty.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Schema.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Statistics.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Subtree.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Subtrees.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Tile.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/generated/include/Cesium3DTiles/Tileset.h"
    "/home/betto/src/cesium-native-ooc/Cesium3DTiles/include/Cesium3DTiles/Library.h"
    )
endif()

