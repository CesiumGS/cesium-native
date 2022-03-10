include(GNUInstallDirs)

option(CESIUM_USE_CONAN_PACKAGES "Whether to resolve other cesium-native libraries with Conan." OFF)
option(CESIUM_TESTS_ENABLED "Whether to enable tests" ON)
option(CESIUM_INSTALL_REQUIREMENTS "Whether to install third-party dependencies in addition to cesium-native libraries" OFF)

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
  cesium_glob_files(
    PUBLIC_HEADERS
    include/${ARG_NAME}/*.h
    include/${ARG_NAME}/Impl/*.h
    generated/include/${ARG_NAME}/*.h
    generated/include/${ARG_NAME}/Impl/*.h
  )

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

  cesium_tests("${ARG_NAME}" "${TESTS}")

  set(CURRENT_ACCESS "PRIVATE")
  foreach(package ${ARG_REQUIRES})
    if(package STREQUAL "PUBLIC" OR package STREQUAL "PRIVATE" OR package STREQUAL "TEST")
      set(CURRENT_ACCESS "${package}")
      continue()
    endif()

    # Don't load TEST packages if tests are disabled
    if(CURRENT_ACCESS STREQUAL "TEST" AND NOT CESIUM_TESTS_ENABLED)
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
      cesium_install_requirement(${TARGET_NAME} ${CURRENT_ACCESS})
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
  )

  # We can't use install(TARGETS x PUBLIC_HEADER DESTINATION y) because PUBLIC_HEADER copies all
  # the files into the DESTINATION without preserving subdirectories. Copy the files manually instead.
  if (EXISTS "generated/include")
    install(DIRECTORY generated/include/ TYPE INCLUDE)
  endif()
  install(DIRECTORY include/ TYPE INCLUDE)

  add_library("${ARG_NAME}::${ARG_NAME}" ALIAS "${ARG_NAME}")
endfunction()

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)

        # Don't warn about missing PDBs, because this is normal for Debug libraries from Conan.
        target_link_options(${targetName} PRIVATE /ignore:4099)
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
    find_package(Catch2 REQUIRED)

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
        Catch2::Catch2
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

function(cesium_install_requirement_configuration configuration libraries access)
  foreach(library ${libraries})
    # If library isn't a really target, it's probably a system library or something. Ignore it.
    if (NOT TARGET ${library})
      continue()
    endif()

    if (library MATCHES "^CONAN_LIB")
      # This is a Conan micro-target, as defined in cmakedeps_macros.cmake conan_package_library_targets.
      # So we can get the actual binary from its IMPORTED_LOCATION property.
      # But first make sure we haven't already installed this file.
      get_target_property(FILE_INSTALLED ${library} FILE_INSTALLED)
      if (NOT FILE_INSTALLED)
        set_target_properties(${library} PROPERTIES FILE_INSTALLED true)
        get_target_property(LIBRARY_PATH ${library} IMPORTED_LOCATION)
        # message("## ${configuration}: Microtarget ${library} ${LIBRARY_PATH}")
        install(FILES "$<$<CONFIG:${configuration}>:${LIBRARY_PATH}>" TYPE LIB)
      endif()
    else()
      # This is a regular target, so recurse.
      # Assumption: we don't need headers for our dependency's dependencies
      # message("## ${configuration}: ${library}")
      cesium_install_requirement(${library} "PRIVATE" "${access}")
    endif()
  endforeach()
endfunction()

# Gets the configurations X for which a given value has $<$<CONFIG:X>:Y> generator expressions
function(cesium_get_value_configurations value outputVariable)
  set(input "${value}")
  set(output "")
  set(CONFIGURATION_REGEX "\\$<\\$<CONFIG:([^>]+)>:([^>]*)>")

  while (input MATCHES ${CONFIGURATION_REGEX})
    # Remove the matched part
    string(LENGTH "${CMAKE_MATCH_0}" MATCH_LENGTH)
    string(SUBSTRING "${input}" ${MATCH_LENGTH} -1 input)
    # MATCH_1: configuration, MATCH_2: value for configuration
    list(APPEND output "${CMAKE_MATCH_1}")
  endwhile()

  set(${outputVariable} "${output}" PARENT_SCOPE)
endfunction()

# Gets the value of a property by evaluating $<$<CONFIG:X>:Y> generator
# expressions for X=configuration and removing all others. The relative order
# of generator and non-generator list elements is not preserved.
function(cesium_get_configuration_value value configuration outputVariable)
  set(input "${value}")
  string(GENEX_STRIP "${value}" output)
  set(CONFIGURATION_REGEX "\\$<\\$<CONFIG:${configuration}>:([^>]*)>")

  while (input MATCHES ${CONFIGURATION_REGEX})
    # Remove the matched part
    string(LENGTH "${CMAKE_MATCH_0}" MATCH_LENGTH)
    string(SUBSTRING "${input}" ${MATCH_LENGTH} -1 input)
    # MATCH_1: value for configuration
    list(APPEND output "${CMAKE_MATCH_1}")
  endwhile()

  set(${outputVariable} "${output}" PARENT_SCOPE)
endfunction()

function(cesium_install_requirement requirement access)
  if (NOT CESIUM_INSTALL_REQUIREMENTS)
    return()
  endif()

  # Copy includes for public libraries
  if (access STREQUAL "PUBLIC")
    get_target_property(INCLUDE_DIRS "${requirement}" INTERFACE_INCLUDE_DIRECTORIES)
    cesium_get_value_configurations("${INCLUDE_DIRS}" INCLUDE_CONFIGS)
    foreach (INCLUDE_CONFIG ${INCLUDE_CONFIGS})
      cesium_get_configuration_value("${INCLUDE_DIRS}" "${INCLUDE_CONFIG}" CONFIG_INCLUDES)
      foreach (INCLUDE ${CONFIG_INCLUDES})
        install(DIRECTORY "$<$<CONFIG:${INCLUDE_CONFIG}>:${INCLUDE}/>" TYPE INCLUDE)
      endforeach()
    endforeach()
    # message("****** ${INCLUDE_CONFIGS}")
  endif()

  # Copy libs for all libraries
  get_target_property(LIBRARIES "${requirement}" INTERFACE_LINK_LIBRARIES)
  # message("***** ${LIBRARIES}")
  string(GENEX_STRIP "${LIBRARIES}" NOT_CONFIG_SPECIFIC)
  cesium_install_requirement_configuration("ALL" "${NOT_CONFIG_SPECIFIC}" "${access}")

  set(CONFIGURATION_REGEX "\\$<\\$<CONFIG:([^>]+)>:([^>]*)>")
  while (LIBRARIES MATCHES ${CONFIGURATION_REGEX})
    # Remove the matched part
    string(LENGTH "${CMAKE_MATCH_0}" MATCH_LENGTH)
    string(SUBSTRING "${LIBRARIES}" ${MATCH_LENGTH} -1 LIBRARIES)
    # MATCH_1: configuration, MATCH_2: library list for configuration
    cesium_install_requirement_configuration("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}" "${access}")
  endwhile()
endfunction()
