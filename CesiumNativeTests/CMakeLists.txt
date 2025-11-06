add_executable(cesium-native-tests "")
set_property(TARGET cesium-native-tests PROPERTY FOLDER "Tests")
configure_cesium_library(cesium-native-tests)

# Add tests here, ensure they define the TEST_SOURCES / TEST_HEADERS
# properties.
set(cesium_native_targets
    Cesium3DTiles
    Cesium3DTilesContent
    Cesium3DTilesReader
    Cesium3DTilesSelection
    Cesium3DTilesWriter
    CesiumAsync
    CesiumGeometry
    CesiumGeospatial
    CesiumGltf
    CesiumGltfContent
    CesiumGltfReader
    CesiumGltfWriter
    CesiumIonClient
    CesiumITwinClient
    CesiumVectorData
    CesiumQuantizedMeshTerrain
    CesiumRasterOverlays
    CesiumUtility
)

if(NOT CESIUM_DISABLE_CURL) 
    list(APPEND cesium_native_targets CesiumCurl)
endif()

cesium_glob_files(test_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(test_headers
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumNativeTests/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumNativeTests/*.h
)
set(test_include_directories ${CMAKE_CURRENT_LIST_DIR}/include)

if(CESIUM_TARGET_WASM)
    target_link_options(
        cesium-native-tests
        PRIVATE
        "-sEXIT_RUNTIME=1" 
    )
endif()

# Iterate through all targets, extracting their private sources and test sources / test headers
foreach(target ${cesium_native_targets})
    get_target_property(target_test_sources ${target} TEST_SOURCES)
    get_target_property(target_test_headers ${target} TEST_HEADERS)

    if ("${target_test_sources}" MATCHES ".*NOTFOUND$")
        message(FATAL_ERROR "${target} did not define TEST_SOURCES property. Use quotes to prevent an empty list from generating this error.")
    endif()

    if ("${target_test_headers}" MATCHES ".*NOTFOUND$")
        message(FATAL_ERROR "${target} did not define TEST_HEADERS property. Use quotes to prevent an empty list from generating this error.")
    endif()

    list(APPEND test_sources "${target_test_sources}")
    list(APPEND test_headers "${target_test_headers}")

    # Workaround to extract the private include directories from a target.
    # (public ∪ private) - interface = private in CMake
    get_target_property(_public_private_include_directories ${target} INCLUDE_DIRECTORIES)
    get_target_property(_interface_include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)
    set(_private_include_directories "")
    list(APPEND _private_include_directories ${_public_private_include_directories})
    list(REMOVE_ITEM _private_include_directories ${_interface_include_directories})
    list(APPEND test_include_directories ${_private_include_directories})

    # Add hardcoded defines to test data directories if they defined
    # the `TEST_DATA_DIR` property on their targets.
    get_target_property(target_test_data_dir ${target} TEST_DATA_DIR)
    if (NOT "${target_test_data_dir}" MATCHES ".*NOTFOUND$")
        target_compile_definitions(
            cesium-native-tests
            PRIVATE
                ${target}_TEST_DATA_DIR=\"${target_test_data_dir}\"
        )
    endif()
endforeach()

if(CESIUM_TARGET_WASM)
  file(GLOB directories_list LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/Cesium*")
  set(FILE_PACKAGER_ARGS "")
  set(TEST_DATA_FILES "")
  foreach(dir ${directories_list})
    if(IS_DIRECTORY ${dir}/test/data)
      list(APPEND FILE_PACKAGER_ARGS "--embed" "${dir}/test/data")

      file(GLOB_RECURSE data_files "${dir}/test/data/*")
      list(APPEND TEST_DATA_FILES ${data_files})
    endif()
  endforeach()

  list(APPEND FILE_PACKAGER_ARGS "--embed" "${CMAKE_SOURCE_DIR}/data")

  set(CESIUM_NATIVE_TESTS_DATA_OBJ ${CMAKE_CURRENT_BINARY_DIR}/cesium-native-tests-data.o)
  add_custom_command(
    COMMENT "Generating test data object file"
    OUTPUT ${CESIUM_NATIVE_TESTS_DATA_OBJ}
    COMMAND ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager none.data ${FILE_PACKAGER_ARGS} --obj-output=${CESIUM_NATIVE_TESTS_DATA_OBJ}
    VERBATIM
    DEPENDS ${TEST_DATA_FILES}
  )
  add_custom_target(
    cesium-native-tests-data
    DEPENDS ${CESIUM_NATIVE_TESTS_DATA_OBJ})
  
  list(APPEND cesium_native_targets "${CESIUM_NATIVE_TESTS_DATA_OBJ}")
endif()

target_sources(
    cesium-native-tests
    PRIVATE
        ${test_sources}
        ${test_headers}
        src/test-main.cpp
)

target_include_directories(
    cesium-native-tests
    PRIVATE
        ${test_include_directories}
)

target_link_libraries(cesium-native-tests
PUBLIC
    doctest::doctest
    ${cesium_native_targets}
PRIVATE
    s2::s2
)

target_compile_definitions(cesium-native-tests
PRIVATE
  CESIUM_NATIVE_DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\"
)

include(CTest)
include(doctest)
if(NOT CESIUM_TARGET_WASM)
    # doctest_discover_tests can't handle the target being an html file, so we just avoid it on a wasm build
    doctest_discover_tests(cesium-native-tests)
endif()
