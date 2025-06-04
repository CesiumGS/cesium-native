add_executable(
    SampleTerrainToHeightmap
    "src/sampleTerrainToHeightmap.cpp"
    "src/ThreadPoolTaskProcessor.cpp"
)

target_link_libraries(
    SampleTerrainToHeightmap
    PRIVATE
        Cesium3DTilesSelection
        CesiumCurl
)
