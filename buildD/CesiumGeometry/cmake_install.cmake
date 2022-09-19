# Install script for directory: /home/betto/src/cesium-native-ooc/CesiumGeometry

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/CesiumGeometry/libCesiumGeometryd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CesiumGeometry" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Availability.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Axis.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/AxisAlignedBox.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/AxisTransforms.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/BoundingSphere.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/CullingResult.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/CullingVolume.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/IntersectionTests.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Library.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/OctreeAvailability.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/OctreeTileID.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/OctreeTilingScheme.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/OrientedBoundingBox.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Plane.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/QuadtreeAvailability.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/QuadtreeRectangleAvailability.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/QuadtreeTileID.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/QuadtreeTileRectangularRange.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/QuadtreeTilingScheme.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Ray.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/Rectangle.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/TileAvailabilityFlags.h"
    "/home/betto/src/cesium-native-ooc/CesiumGeometry/include/CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h"
    )
endif()

