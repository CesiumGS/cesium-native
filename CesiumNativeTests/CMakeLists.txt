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
    CesiumQuantizedMeshTerrain
    CesiumRasterOverlays
    CesiumUtility
)

if (NOT CESIUM_TARGET_WASM)
    list(APPEND cesium_native_targets CesiumVectorData) # blend2d doesn't support wasm builds
endif()

if(NOT CESIUM_DISABLE_CURL AND NOT CESIUM_TARGET_WASM) 
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
  foreach(dir ${directories_list})
    if(IS_DIRECTORY ${dir}/test/data)
      list(APPEND CESIUM_TEST_DATA_DIRS --embed)
      list(APPEND CESIUM_TEST_DATA_DIRS "${dir}/test/data")
    endif()
  endforeach()

  list(APPEND CESIUM_TEST_DATA_DIRS --embed)
  list(APPEND CESIUM_TEST_DATA_DIRS "${CMAKE_SOURCE_DIR}/data")

  message("${CESIUM_TEST_DATA_DIRS}")

  set(CESIUM_NATIVE_TESTS_DATA_OBJ ${CMAKE_CURRENT_BINARY_DIR}/cesium-native-tests-data.o)
  add_custom_target(
    cesium-native-tests-data
    COMMAND ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager none.data ${CESIUM_TEST_DATA_DIRS} --obj-output=${CESIUM_NATIVE_TESTS_DATA_OBJ})

  add_library(cesium-native-tests-data-lib OBJECT IMPORTED)
  add_dependencies(cesium-native-tests-data-lib cesium-native-tests-data)
  set_property(TARGET cesium-native-tests-data-lib PROPERTY IMPORTED_OBJECTS
    "${CESIUM_NATIVE_TESTS_DATA_OBJ}")

  list(APPEND cesium_native_targets cesium-native-tests-data-lib)
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
  CESIUM_NATIVE_DATA_DIR=\"${CMAKE_SOURCE_DIR}/../data\"
)

include(CTest)
include(doctest)
doctest_discover_tests(cesium-native-tests)

if(NOT CESIUM_TARGET_WASM)
    # doctest_discover_tests can't handle the target being an html file, so we just avoid it on a wasm build
    doctest_discover_tests(cesium-native-tests)
endif()
