cmake_minimum_required(VERSION 3.15)
project(CesiumGltfReader CXX)

include(cesium)

cesium_library(
  NAME CesiumGltfReader
  REQUIRES
    PUBLIC
      CesiumAsync::CesiumAsync
      CesiumGltf::CesiumGltf
      CesiumJsonReader::CesiumJsonReader
      CesiumUtility::CesiumUtility
    PRIVATE
      base64::base64
      draco::draco
      stb::stb
      ktx::libktx@Ktx
)
