add_library(CesiumGltf "")

configure_cesium_library(CesiumGltf)

file(GLOB SOURCES include/CesiumGltf/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGltf/*.h)

target_sources(
    CesiumGltf
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGltf
    PUBLIC
        include
    PRIVATE
        src
    SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/cgltf
)

target_link_libraries(CesiumGltf PUBLIC CesiumGeometry CesiumUtility simdjson)
