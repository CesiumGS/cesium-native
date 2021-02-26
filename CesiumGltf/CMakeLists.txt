add_library(CesiumGltf "")
configure_cesium_library(CesiumGltf)

set(CESIUM_GLTF_PUBLIC_HEADERS
    include/CesiumGltf/Accessor.h
    include/CesiumGltf/AccessorSparse.h
    include/CesiumGltf/AccessorSparseIndices.h
    include/CesiumGltf/AccessorSparseValues.h
    include/CesiumGltf/AccessorSpec.h
    include/CesiumGltf/AccessorView.h
    include/CesiumGltf/AccessorWriter.h
    include/CesiumGltf/AnimationChannel.h
    include/CesiumGltf/AnimationChannelTarget.h
    include/CesiumGltf/Animation.h
    include/CesiumGltf/AnimationSampler.h
    include/CesiumGltf/Asset.h
    include/CesiumGltf/BufferCesium.h
    include/CesiumGltf/Buffer.h
    include/CesiumGltf/BufferSpec.h
    include/CesiumGltf/BufferView.h
    include/CesiumGltf/Camera.h
    include/CesiumGltf/CameraOrthographic.h
    include/CesiumGltf/CameraPerspective.h
    include/CesiumGltf/ExtensibleObject.h
    include/CesiumGltf/ImageCesium.h
    include/CesiumGltf/Image.h
    include/CesiumGltf/ImageSpec.h
    include/CesiumGltf/JsonValue.h
    include/CesiumGltf/KHR_draco_mesh_compression.h
    include/CesiumGltf/Library.h
    include/CesiumGltf/Material.h
    include/CesiumGltf/MaterialNormalTextureInfo.h
    include/CesiumGltf/MaterialOcclusionTextureInfo.h
    include/CesiumGltf/MaterialPBRMetallicRoughness.h
    include/CesiumGltf/Mesh.h
    include/CesiumGltf/MeshPrimitive.h
    include/CesiumGltf/Model.h
    include/CesiumGltf/ModelSpec.h
    include/CesiumGltf/NamedObject.h
    include/CesiumGltf/Node.h
    include/CesiumGltf/Sampler.h
    include/CesiumGltf/Scene.h
    include/CesiumGltf/Skin.h
    include/CesiumGltf/Texture.h
    include/CesiumGltf/TextureInfo.h
)

set(CESIUM_GLTF_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

set(CESIUM_GLTF_SOURCE
    src/Accessor.cpp
    src/JsonValue.cpp
    src/Model.cpp
)

set(CESIUM_GLTF_TEST_SOURCE
    test/TestAccessorView.cpp
    test/TestJsonValue.cpp
)

list(TRANSFORM CESIUM_GLTF_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_GLTF_TEST_SOURCE "${CESIUM_GLTF_TEST_SOURCE}" PARENT_SCOPE)

target_sources(
    CesiumGltf
    PRIVATE
        ${CESIUM_GLTF_SOURCE}
    PUBLIC
        ${CESIUM_GLTF_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltf
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
    PUBLIC
        ${CESIUM_GLTF_PUBLIC_INCLUDE_DIR}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/src"
)

target_link_libraries(CesiumGltf 
    PUBLIC 
        GSL
)
