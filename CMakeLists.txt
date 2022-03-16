cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")

option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)
option(CESIUM_CONAN_INSTALL "Whether to install dependencies from Conan" ON)
option(CESIUM_CONAN_LESS_CONFIGS "Whether to use the Release configuration of Conan dependencies in the MinSizeRel and RelWithDebInfo configurations" ON)

if (CESIUM_TESTS_ENABLED)
  enable_testing()
endif()

if (CESIUM_CONAN_INSTALL)
  include(ConfigureConan)
endif()

include(cesium)

file(GLOB libraryFiles LIST_DIRECTORIES true "${CMAKE_CURRENT_LIST_DIR}/Cesium*")
foreach(library ${libraryFiles})
  if(IS_DIRECTORY "${library}" AND EXISTS "${library}/library.yml")
    add_subdirectory(${library})
  endif()
endforeach()

add_subdirectory(doc)
