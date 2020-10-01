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
        # We're not using CMake for tinygltf at all, but it's header only. Add its headers here.
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/tinygltf

        # Draco's CMake doesn't use target_include_directories, so we need to manually add draco's include dirs here.
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/draco/src
        ${CMAKE_CURRENT_BINARY_DIR}/..
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(Cesium3DTiles PUBLIC CesiumGeospatial CesiumGeometry CesiumUtility uriparser draco)
