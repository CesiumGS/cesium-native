add_library(CesiumGltfWriter "")

configure_cesium_library(CesiumGltfWriter)

#file(GLOB SOURCES include/CesiumGltf/*.h src/*.cpp src/*.h generated/*.cpp generated/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGltf/*.h)

set(SOURCES 
    src/AccessorSparseWriter.cpp
    src/AccessorWriter.cpp
    src/AnimationWriter.cpp
    src/AssetWriter.cpp
    src/BufferWriter.cpp
    src/BufferViewWriter.cpp
    src/CameraWriter.cpp
    src/ImageWriter.cpp
    src/MaterialWriter.cpp
    src/NodeWriter.cpp
    src/SamplerWriter.cpp
    src/SceneWriter.cpp
    src/SkinWriter.cpp
    src/GLBWriter.cpp
    src/Writer.cpp
)

target_sources(
    CesiumGltfWriter
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGltfWriter
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/rapidjson/include
        ${CMAKE_BINARY_DIR}
    PUBLIC
        include
    PRIVATE
        ${MAGIC_ENUM_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb
        src
        generated
)

target_link_libraries(CesiumGltfWriter PUBLIC CesiumGltf)
