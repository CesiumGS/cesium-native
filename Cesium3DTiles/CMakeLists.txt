add_library(Cesium3DTiles "")

configure_cesium_library(Cesium3DTiles)

file(GLOB SOURCES include/Cesium3DTiles/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/Cesium3DTiles/*.h)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/rapidjson/include

        # Draco's CMake doesn't use target_include_directories, so we need to manually add draco's include dirs here.
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/draco/src

        ${CMAKE_BINARY_DIR}
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(Cesium3DTiles PUBLIC CesiumAsync CesiumGeospatial CesiumGeometry CesiumGltf CesiumGltfReader CesiumUtility uriparser draco tinyxml2 spdlog)
