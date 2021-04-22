add_library(CesiumGltfWriter "")

configure_cesium_library(CesiumGltfWriter)

cesium_glob_files(CESIUM_GLTF_WRITER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GLTF_WRITER_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_GLTF_WRITER_PUBLIC_HEADERS include/CesiumGltf/*.h)
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_HEADERS test/*.h)

set_target_properties(CesiumGltfWriter
    PROPERTIES 
        TEST_SOURCES "${CESIUM_GLTF_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_WRITER_TEST_HEADERS}"
)

target_sources(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_GLTF_WRITER_SOURCES}
        ${CESIUM_GLTF_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_WRITER_PUBLIC_HEADERS}
)

target_compile_definitions(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_DEFINES}
)

target_include_directories(
    CesiumGltfWriter
    SYSTEM PUBLIC
        # draco
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(CesiumGltfWriter 
    PUBLIC 
        CesiumGltf 
        modp_b64
    PRIVATE
        magic_enum
)
