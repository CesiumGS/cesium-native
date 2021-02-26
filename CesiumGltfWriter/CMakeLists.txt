add_library(CesiumGltfWriter "")

configure_cesium_library(CesiumGltfWriter)

set(CESIUM_GLTF_WRITER_SOURCE 
    src/AccessorSparseWriter.cpp
    src/AccessorWriter.cpp
    src/AnimationWriter.cpp
    src/AssetWriter.cpp
    src/BufferWriter.cpp
    src/BufferViewWriter.cpp
    src/CameraWriter.cpp
    src/ImageWriter.cpp
    src/MaterialWriter.cpp
    src/MeshWriter.cpp
    src/NodeWriter.cpp
    src/SamplerWriter.cpp
    src/SceneWriter.cpp
    src/SkinWriter.cpp
    src/TextureWriter.cpp
    src/JsonObjectWriter.cpp
    src/JsonWriter.cpp
    src/PrettyJsonWriter.cpp
    src/ExtensionWriter.cpp
    src/WriteBinaryGLB.cpp
    src/Writer.cpp
)

set(CESIUM_GLTF_WRITER_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

set(CESIUM_GLTF_WRITER_TEST_SOURCE 
    test/TestFindMinMax.cpp
    test/TestJsonObjectWriter.cpp
    test/TestWriter.cpp
)

list(TRANSFORM CESIUM_GLTF_WRITER_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_GLTF_WRITER_TEST_SOURCE "${CESIUM_GLTF_WRITER_TEST_SOURCE}" PARENT_SCOPE)

target_sources(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_GLTF_WRITER_SOURCE}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_compile_definitions(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_DEFINES}
)

target_include_directories(
    CesiumGltfWriter
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
    PUBLIC
        ${CESIUM_GLTF_PUBLIC_HEADERS}
        ${CESIUM_GLTF_WRITER_PUBLIC_INCLUDE_DIR}
    PRIVATE
        ${CESIUM_NATIVE_BASE64_ENCODE_INCLUDE_DIR}
        ${CESIUM_NATIVE_MAGIC_ENUM_INCLUDE_DIR}
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        "${CMAKE_CURRENT_LIST_DIR}/src"
        "${CMAKE_CURRENT_LIST_DIR}/generated"
)

target_link_libraries(CesiumGltfWriter 
    PUBLIC 
        CesiumGltf 
)
