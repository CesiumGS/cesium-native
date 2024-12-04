add_library(CesiumGltfContent "")
configure_cesium_library(CesiumGltfContent)

cesium_glob_files(CESIUM_GLTF_CONTENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_CONTENT_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_CONTENT_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfContent/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltfContent/*.h
)
cesium_glob_files(CESIUM_GLTF_CONTENT_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_GLTF_CONTENT_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumGltfContent
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_CONTENT_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_CONTENT_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumGltfContent
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_CONTENT_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltfContent
    PRIVATE
        ${CESIUM_GLTF_CONTENT_SOURCES}
        ${CESIUM_GLTF_CONTENT_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_CONTENT_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumGltfContent
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumGltfContent
    PUBLIC
        CesiumAsync
        CesiumGeometry
        CesiumGeospatial
        CesiumGltf
        CesiumGltfReader
        CesiumUtility
)
