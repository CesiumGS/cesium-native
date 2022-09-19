# Install script for directory: /home/betto/src/cesium-native-ooc/extern/libmorton

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libmorton" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton_common.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton_AVX512BITALG.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton_BMI.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton2D_LUTs.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton2D.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton3D_LUTs.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton3D.h"
    "/home/betto/src/cesium-native-ooc/extern/libmorton/include/libmorton/morton.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton/libmortonTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton/libmortonTargets.cmake"
         "/home/betto/src/cesium-native-ooc/buildD/extern/libmorton/CMakeFiles/Export/share/cmake/libmorton/libmortonTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton/libmortonTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton/libmortonTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/libmorton/CMakeFiles/Export/share/cmake/libmorton/libmortonTargets.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/libmorton" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/buildD/extern/libmorton/libmortonConfigVersion.cmake"
    "/home/betto/src/cesium-native-ooc/buildD/extern/libmorton/libmortonConfig.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/libmorton/libmorton.pc")
endif()

