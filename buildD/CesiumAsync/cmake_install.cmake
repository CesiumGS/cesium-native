# Install script for directory: /home/betto/src/cesium-native-ooc/CesiumAsync

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/CesiumAsync/libCesiumAsyncd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CesiumAsync" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/AsyncSystem.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/CacheItem.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/CachingAssetAccessor.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/Future.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/HttpHeaders.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/IAssetAccessor.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/IAssetRequest.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/IAssetResponse.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/ICacheDatabase.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/ITaskProcessor.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/Library.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/Promise.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/SharedFuture.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/SqliteCache.h"
    "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/ThreadPool.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CesiumAsync" TYPE DIRECTORY FILES "/home/betto/src/cesium-native-ooc/CesiumAsync/include/CesiumAsync/Impl")
endif()

