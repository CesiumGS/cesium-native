# Install script for directory: /home/betto/src/cesium-native-ooc/extern/uriparser

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/liburiparserd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/uriparser" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/UriBase.h"
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/UriDefsAnsi.h"
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/UriDefsConfig.h"
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/UriDefsUnicode.h"
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/Uri.h"
    "/home/betto/src/cesium-native-ooc/extern/uriparser/include/uriparser/UriIp4.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/cmake/uriparser-config.cmake"
    "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/cmake/uriparser-config-version.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6/uriparser.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6/uriparser.cmake"
         "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/CMakeFiles/Export/lib/cmake/uriparser-0.9.6/uriparser.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6/uriparser-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6/uriparser.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/CMakeFiles/Export/lib/cmake/uriparser-0.9.6/uriparser.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/uriparser-0.9.6" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/CMakeFiles/Export/lib/cmake/uriparser-0.9.6/uriparser-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/uriparser/liburiparser.pc")
endif()

