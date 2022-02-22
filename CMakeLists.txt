cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX C
)

enable_testing()
option(CESIUM_TESTS_ENABLED "Whether to enable tests" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")
include(cesium)

file(GLOB libraryFiles LIST_DIRECTORIES true "${CMAKE_CURRENT_LIST_DIR}/Cesium*")
foreach(library ${libraryFiles})
  if(IS_DIRECTORY "${library}" AND EXISTS "${library}/library.yml")
    add_subdirectory(${library})
  endif()
endforeach()

add_subdirectory(doc)
