add_library(CesiumGltfReader "")
configure_cesium_library(CesiumGltfReader)

set(CESIUM_GLTF_READER_SOURCE
    generated/AccessorJsonHandler.cpp
    generated/AccessorSparseIndicesJsonHandler.cpp
    generated/AccessorSparseJsonHandler.cpp
    generated/AccessorSparseValuesJsonHandler.cpp
    generated/AnimationChannelJsonHandler.cpp
    generated/AnimationChannelTargetJsonHandler.cpp
    generated/AnimationJsonHandler.cpp
    generated/AnimationSamplerJsonHandler.cpp
    generated/AssetJsonHandler.cpp
    generated/BufferJsonHandler.cpp
    generated/BufferViewJsonHandler.cpp
    generated/CameraJsonHandler.cpp
    generated/CameraOrthographicJsonHandler.cpp
    generated/CameraPerspectiveJsonHandler.cpp
    generated/ImageJsonHandler.cpp
    generated/KHR_draco_mesh_compressionJsonHandler.cpp
    generated/MaterialJsonHandler.cpp
    generated/MaterialNormalTextureInfoJsonHandler.cpp
    generated/MaterialOcclusionTextureInfoJsonHandler.cpp
    generated/MaterialPBRMetallicRoughnessJsonHandler.cpp
    generated/MeshJsonHandler.cpp
    generated/MeshPrimitiveJsonHandler.cpp
    generated/ModelJsonHandler.cpp
    generated/NodeJsonHandler.cpp
    generated/SamplerJsonHandler.cpp
    generated/SceneJsonHandler.cpp
    generated/SkinJsonHandler.cpp
    generated/TextureInfoJsonHandler.cpp
    generated/TextureJsonHandler.cpp
    src/BoolJsonHandler.cpp
    src/decodeDataUrls.cpp
    src/decodeDraco.cpp
    src/DoubleJsonHandler.cpp
    src/ExtensibleObjectJsonHandler.cpp
    src/IgnoreValueJsonHandler.cpp
    src/JsonHandler.cpp
    src/JsonObjectJsonHandler.cpp
    src/NamedObjectJsonHandler.cpp
    src/ObjectJsonHandler.cpp
    src/Reader.cpp
    src/StringJsonHandler.cpp
)

set(CESIUM_GLTF_READER_HEADERS
    generated/AccessorJsonHandler.h
    generated/AccessorSparseIndicesJsonHandler.h
    generated/AccessorSparseJsonHandler.h
    generated/AccessorSparseValuesJsonHandler.h
    generated/AnimationChannelJsonHandler.h
    generated/AnimationChannelTargetJsonHandler.h
    generated/AnimationJsonHandler.h
    generated/AnimationSamplerJsonHandler.h
    generated/AssetJsonHandler.h
    generated/BufferJsonHandler.h
    generated/BufferViewJsonHandler.h
    generated/CameraJsonHandler.h
    generated/CameraOrthographicJsonHandler.h
    generated/CameraPerspectiveJsonHandler.h
    generated/ImageJsonHandler.h
    generated/KHR_draco_mesh_compressionJsonHandler.h
    generated/MaterialJsonHandler.h
    generated/MaterialNormalTextureInfoJsonHandler.h
    generated/MaterialOcclusionTextureInfoJsonHandler.h
    generated/MaterialPBRMetallicRoughnessJsonHandler.h
    generated/MeshJsonHandler.h
    generated/MeshPrimitiveJsonHandler.h
    generated/ModelJsonHandler.h
    generated/NodeJsonHandler.h
    generated/SamplerJsonHandler.h
    generated/SceneJsonHandler.h
    generated/SkinJsonHandler.h
    generated/TextureInfoJsonHandler.h
    generated/TextureJsonHandler.h
    src/ArrayJsonHandler.h
    src/BoolJsonHandler.h
    src/decodeDataUrls.h
    src/decodeDraco.h
    src/DictionaryJsonHandler.h
    src/DoubleJsonHandler.h
    src/ExtensibleObjectJsonHandler.h
    src/IgnoreValueJsonHandler.h
    src/IJsonHandler.h
    src/IntegerJsonHandler.h
    src/JsonHandler.h
    src/JsonObjectJsonHandler.h
    src/NamedObjectJsonHandler.h
    src/ObjectJsonHandler.h
    src/StringJsonHandler.h
)

set(CESIUM_GLTF_READER_TEST_SOURCE
    test/TestReader.cpp
)

list(TRANSFORM CESIUM_GLTF_READER_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_GLTF_READER_TEST_SOURCE "${CESIUM_GLTF_READER_TEST_SOURCE}" PARENT_SCOPE)

set(CESIUM_GLTF_PUBLIC_HEADERS
    include/CesiumGltf/Reader.h
    include/CesiumGltf/ReaderLibrary.h
)

target_sources(
    CesiumGltfReader
    PRIVATE
        ${CESIUM_GLTF_READER_SOURCE}
        ${CESIUM_GLTF_READER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltfReader
    SYSTEM PUBLIC
        ${CMAKE_BINARY_DIR}
    PUBLIC
        include
    PRIVATE
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        src
        generated
)

target_link_libraries(CesiumGltfReader 
    PUBLIC 
        CesiumGltf 
        GSL 
    PRIVATE 
        draco 
        modp_b64
)

