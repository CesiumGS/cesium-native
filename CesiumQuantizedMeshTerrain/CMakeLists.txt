add_library(CesiumQuantizedMeshTerrain "")
configure_cesium_library(CesiumQuantizedMeshTerrain)

cesium_glob_files(CESIUM_QUANTIZED_MESH_TERRAIN_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_QUANTIZED_MESH_TERRAIN_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_QUANTIZED_MESH_TERRAIN_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumQuantizedMeshTerrain/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumQuantizedMeshTerrain/*.h
)
cesium_glob_files(CESIUM_QUANTIZED_MESH_TERRAIN_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_QUANTIZED_MESH_TERRAIN_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumQuantizedMeshTerrain
    PROPERTIES
        TEST_SOURCES "${CESIUM_QUANTIZED_MESH_TERRAIN_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_QUANTIZED_MESH_TERRAIN_TEST_HEADERS}"
)

set_target_properties(CesiumQuantizedMeshTerrain
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_QUANTIZED_MESH_TERRAIN_PUBLIC_HEADERS}"
)

target_sources(
    CesiumQuantizedMeshTerrain
    PRIVATE
        ${CESIUM_QUANTIZED_MESH_TERRAIN_SOURCES}
        ${CESIUM_QUANTIZED_MESH_TERRAIN_HEADERS}
    PUBLIC
        ${CESIUM_QUANTIZED_MESH_TERRAIN_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumQuantizedMeshTerrain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumQuantizedMeshTerrain
    PUBLIC
        CesiumAsync
        CesiumGeospatial
        CesiumGltf
        CesiumGltfContent
        CesiumJsonReader
        CesiumJsonWriter
        CesiumUtility
)
