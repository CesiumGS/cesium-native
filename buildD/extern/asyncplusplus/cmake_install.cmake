# Install script for directory: /home/betto/src/cesium-native-ooc/extern/asyncplusplus

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/asyncplusplus/Async++Config.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/extern/asyncplusplus/libasync++d.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/Async++.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/Async++.cmake"
         "/home/betto/src/cesium-native-ooc/buildD/extern/asyncplusplus/CMakeFiles/Export/cmake/Async++.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/Async++-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/Async++.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/asyncplusplus/CMakeFiles/Export/cmake/Async++.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/buildD/extern/asyncplusplus/CMakeFiles/Export/cmake/Async++-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/async++" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/aligned_alloc.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/cancel.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/continuation_vector.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/parallel_for.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/parallel_invoke.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/parallel_reduce.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/partitioner.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/range.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/ref_count.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/scheduler.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/scheduler_fwd.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/task.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/task_base.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/traits.h"
    "/home/betto/src/cesium-native-ooc/extern/asyncplusplus/include/async++/when_all_any.h"
    )
endif()

