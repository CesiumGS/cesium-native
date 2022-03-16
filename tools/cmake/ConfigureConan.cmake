# cmake-conan configuration

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/conan.cmake)

if (CMAKE_CONFIGURATION_TYPES)
  if (CESIUM_CONAN_LESS_CONFIGS)
    set(CONFIGS "Debug;Release")
  else()
    set(CONFIGS "${CMAKE_CONFIGURATION_TYPES}")
  endif()

  foreach(TYPE ${CONFIGS})
      set(CMAKE_BUILD_TYPE ${TYPE})
      conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})
      conan_cmake_install(
        PATH_OR_REFERENCE "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.py"
        BUILD outdated
        BUILD cascade
        #REMOTE conan-center
        SETTINGS ${settings})
      unset(CMAKE_BUILD_TYPE)
  endforeach()
else()
  conan_cmake_autodetect(settings)
  conan_cmake_install(
    PATH_OR_REFERENCE "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.py"
    BUILD outdated
    BUILD cascade
    #REMOTE conan-center
    SETTINGS ${settings})
endif()

# conan_cmake_install(PATH_OR_REFERENCE .
#                     BUILD missing
#                     REMOTE conan-center
#                     SETTINGS ${settings})

