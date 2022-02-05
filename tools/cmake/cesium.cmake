include(GNUInstallDirs)

option(CESIUM_USE_CONAN_PACKAGES "Whether to add ${CMAKE_BINARY_DIR}/conan to the module and prefix paths in order to resolve packages installed by Conan." on)

# Tell CMake to look for packages from Conan
# if (CESIUM_USE_CONAN_PACKAGES)
#   if (NOT "${CMAKE_BINARY_DIR}/conan" IN_LIST CMAKE_MODULE_PATH)
#     list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/conan")
#   endif()
#   if (NOT "${CMAKE_BINARY_DIR}/conan" IN_LIST CMAKE_PREFIX_PATH)
#     list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/conan")
#   endif()
# endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/conan")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/conan")

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

    if (NOT ${targetName} MATCHES "-tests")
        string(TOUPPER ${targetName} capitalizedTargetName)
        target_compile_definitions(
            ${targetName}
            PRIVATE
                ${capitalizedTargetName}_BUILDING
        )


    endif()
endfunction()

# Workaround for targets that erroneously forget to
# declare their include directories as `SYSTEM`
function(target_link_libraries_system target scope)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)

    if ("${lib_include_dirs}" MATCHES ".*NOTFOUND$")
        message(FATAL_ERROR "${target}: Cannot use INTERFACE_INCLUDE_DIRECTORIES from target ${lib} as it does not define it")
    endif()

    target_include_directories(${target} SYSTEM ${scope} ${lib_include_dirs})
    target_link_libraries(${target} ${scope} ${lib})
  endforeach()
endfunction()

macro(cesium_tests target sources)
  find_package(catch2 REQUIRED)

  add_executable(${target}-tests "")
  configure_cesium_library(${target}-tests)

  target_sources(
    ${target}-tests
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/../CesiumNativeTests/src/test-main.cpp
      ${sources}
  )

  # Allow tests access to the library's private headers
  target_include_directories(
    ${target}-tests
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/src
      ${CMAKE_CURRENT_LIST_DIR}/generated/src
  )

  target_link_libraries(
    ${target}-tests
    PRIVATE
    ${target}
      catch2::catch2
  )

  target_compile_definitions(
    ${target}-tests
    PRIVATE
        ${target}_TEST_DATA_DIR=\"${CMAKE_CURRENT_LIST_DIR}/test/data\"
  )

  include(CTest)
  include(Catch)
  catch_discover_tests(${target}-tests TEST_PREFIX "${target}: ")
endmacro()
