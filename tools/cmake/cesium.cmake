include(GNUInstallDirs)

option(CESIUM_USE_CONAN_PACKAGES "Whether to resolve other cesium-native libraries with Conan." OFF)
option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)

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

# Parameters are:
# - NAME: the name of the library target to create (required)
# - REQUIRES: The list of libraries to depend on, further grouped into PUBLIC, PRIVATE, and TEST sections.
#             PUBLIC dependencies are visible in the public interface of this library. PRIVATE ones are not.
#             TEST dependencies are only required to build the tests, not the library itself.
#             Each library is specified in the format "Namespace::Target@Package". "Package" is passed to
#             find_package. "Namespace::Target" is passed to target_link_libraries. If there's no
#             "@Package" specified, the "Namespace" is used as the package name.
function(cesium_library)
  set(options "")
  set(oneValueArgs NAME)
  set(multiValueArgs REQUIRES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT ARG_NAME)
    message(WARNING "cesium_library must have a NAME")
    return()
  endif()

  cesium_glob_files(SOURCES src/*.c src/*.cpp generated/src/*.c generated/src/*.cpp)
  cesium_glob_files(HEADERS src/*.h src/*.hpp generated/src/*.h generated/src/*.hpp)
  cesium_glob_files(TESTS test/*.cpp test/*.hpp tests/*.h)
  cesium_glob_files(PUBLIC_HEADERS include/${ARG_NAME}/*.h generated/include/${ARG_NAME}/*.h)

  add_library(${ARG_NAME} "")
  configure_cesium_library(${ARG_NAME})

  target_sources(
    ${ARG_NAME}
    PRIVATE
      ${SOURCES}
      ${HEADERS}
    PUBLIC
      ${PUBLIC_HEADERS}
  )

  target_include_directories(
    ${ARG_NAME}
    SYSTEM PUBLIC
      include
      generated/include
    PRIVATE
      src
      generated/src
  )

  set_target_properties(
    ${ARG_NAME}
    PROPERTIES
      PUBLIC_HEADER
        "${PUBLIC_HEADERS}"
  )

  cesium_tests("${ARG_NAME}" "${TESTS}")

  set(CURRENT_ACCESS "PRIVATE")
  foreach(package ${ARG_REQUIRES})
    if(package STREQUAL "PUBLIC" OR package STREQUAL "PRIVATE" OR package STREQUAL "TEST")
      set(CURRENT_ACCESS "${package}")
      continue()
    endif()

    set(PACKAGE_NAME "${package}")
    set(TARGET_NAME "${package}")

    string(FIND "${package}" "@" packageSeparatorPosition)
    string(FIND "${package}" "::" namespaceSeparatorPosition)

    if (${packageSeparatorPosition} GREATER 0)
      # There's an @ at the end of the dependency name, so use the string after it as the package name.
      string(SUBSTRING "${package}" 0 ${packageSeparatorPosition} TARGET_NAME)
      math(EXPR packageSeparatorPosition "${packageSeparatorPosition} + 1")
      string(SUBSTRING "${package}" ${packageSeparatorPosition} -1 PACKAGE_NAME)
    elseif (${namespaceSeparatorPosition} GREATER 0)
      # The package name as a "Namespace::", so use the namespace as the package name
      string(SUBSTRING "${package}" 0 ${namespaceSeparatorPosition} PACKAGE_NAME)
    endif()

    if ("${PACKAGE_NAME}" MATCHES "^Cesium")
      find_cesium_package("${PACKAGE_NAME}")
    else()
      find_package("${PACKAGE_NAME}" REQUIRED)
    endif()

    if (CURRENT_ACCESS STREQUAL "TEST")
      # Only add this dependency to the test project
      if (CESIUM_TESTS_ENABLED AND TESTS)
        target_link_libraries(
          ${ARG_NAME}-tests
          PRIVATE
            ${TARGET_NAME}
        )
      endif()
    else()
      target_link_libraries(
        ${ARG_NAME}
        ${CURRENT_ACCESS}
          ${TARGET_NAME}
      )
    endif()
  endforeach()

  install(TARGETS "${ARG_NAME}"
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ARG_NAME}
  )

  add_library("${ARG_NAME}::${ARG_NAME}" ALIAS "${ARG_NAME}")
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

function(cesium_tests target sources)
  if (CESIUM_TESTS_ENABLED AND sources)
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
  endif()
endfunction()

macro(find_cesium_package package)
  if (CESIUM_USE_CONAN_PACKAGES)
    find_package("${package}" REQUIRED)
  endif()
endmacro()
