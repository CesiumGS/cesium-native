add_library(CesiumGltfReader "")

configure_cesium_library(CesiumGltfReader)

file(GLOB SOURCES include/CesiumGltf/*.h src/*.cpp src/*.h)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/rapidjson/include
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(CesiumGltfReader PUBLIC CesiumGltf)
