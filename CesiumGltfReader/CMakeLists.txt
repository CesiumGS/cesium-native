add_library(CesiumGltfReader "")

configure_cesium_library(CesiumGltfReader)

file(GLOB SOURCES include/CesiumGltf/*.h src/*.cpp src/*.h generated/*.cpp generated/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGltf/*.h)

target_sources(
    CesiumGltfReader
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGltfReader
    SYSTEM PUBLIC
        # Draco's CMake doesn't use target_include_directories, so we need to manually add draco's include dirs here.
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/draco/src
        ${CMAKE_BINARY_DIR}
    PUBLIC
        include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/stb
        src
        generated
)

target_link_libraries(CesiumGltfReader PUBLIC CesiumGltf GSL PRIVATE draco modp_b64)
