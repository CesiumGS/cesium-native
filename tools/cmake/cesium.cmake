include(GNUInstallDirs)

option(CESIUM_USE_CONAN_PACKAGES "Whether to add ${CMAKE_BINARY_DIR}/conan to the module and prefix paths in order to resolve packages installed by Conan." on)

# Tell CMake to look for packages from Conan
if (CESIUM_USE_CONAN_PACKAGES)
  if (NOT "${CMAKE_BINARY_DIR}/conan" IN_LIST CMAKE_MODULE_PATH)
    message("Adding ${CMAKE_BINARY_DIR}/conan")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/conan")
  endif()
  if (NOT "${CMAKE_BINARY_DIR}/conan" IN_LIST CMAKE_PREFIX_PATH)
  message("Adding2 ${CMAKE_BINARY_DIR}/conan")
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/conan")
  endif()
endif()

if (NOT DEFINED GLOB_USE_CONFIGURE_DEPENDS)
    set(GLOB_USE_CONFIGURE_DEPENDS OFF CACHE BOOL
        "Controls if cesium-native targets should use configure_depends or not for globbing their sources"
    )
endif()

function(cesium_glob_files out_var_name regexes)
    set(files "")
    foreach(arg ${ARGV})
        list(APPEND regexes_only "${arg}")
    endforeach()
    list(POP_FRONT regexes_only)
    if (GLOB_USE_CONFIGURE_DEPENDS)
        file(GLOB_RECURSE files CONFIGURE_DEPENDS ${regexes_only})
    else()
        file(GLOB files ${regexes_only})
    endif()
    set(${ARGV0} "${files}" PARENT_SCOPE)
endfunction()

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    target_compile_definitions(
        ${targetName}
        PUBLIC
            GLM_FORCE_XYZW_ONLY # Disable .rgba and .stpq to make it easier to view values from debugger
            GLM_FORCE_EXPLICIT_CTOR # Disallow implicit conversions between dvec3 <-> dvec4, dvec3 <-> fvec3, etc
            GLM_FORCE_SIZE_T_LENGTH # Make vec.length() and vec[idx] use size_t instead of int
    )

    if (BUILD_SHARED_LIBS)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                CESIUM_SHARED=${BUILD_SHARED_LIBS}
        )
    endif()

    if (NOT ${targetName} MATCHES "cesium-native-tests")
        string(TOUPPER ${targetName} capitalizedTargetName)
        target_compile_definitions(
            ${targetName}
            PRIVATE
                ${capitalizedTargetName}_BUILDING
        )


    endif()

endfunction()

macro(cesium_dependencies dependencies)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  conan_cmake_configure(
    REQUIRES
      ms-gsl/3.1.0
      glm/0.9.9.8
      uriparser/0.9.6
    GENERATORS
      CMakeDeps
  )

  foreach(TYPE ${CMAKE_CONFIGURATION_TYPES})
      set(CMAKE_BUILD_TYPE ${TYPE})
      conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})

      conan_cmake_install(PATH_OR_REFERENCE .
                        BUILD missing
                        SETTINGS ${settings})
  endforeach()

  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
  list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})
endmacro()
