# Install script for directory: /home/betto/src/cesium-native-ooc/CesiumJsonReader

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/CesiumJsonReader/libCesiumJsonReaderd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CesiumJsonReader" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/ArrayJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/BoolJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/DictionaryJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/DoubleJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/ExtensibleObjectJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/ExtensionReaderContext.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/ExtensionsJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/IExtensionJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/IJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/IgnoreValueJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/IntegerJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/JsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/JsonObjectJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/JsonReader.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/Library.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/ObjectJsonHandler.h"
    "/home/betto/src/cesium-native-ooc/CesiumJsonReader/include/CesiumJsonReader/StringJsonHandler.h"
    )
endif()

