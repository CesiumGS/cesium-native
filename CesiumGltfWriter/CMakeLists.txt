cmake_minimum_required(VERSION 3.15)
project(CesiumGltfWriter CXX)

include(cesium)

cesium_library(
  NAME CesiumGltfWriter
  REQUIRES
    PUBLIC
      CesiumGltf::CesiumGltf
      CesiumUtility::CesiumUtility
      CesiumJsonWriter::CesiumJsonWriter
    PRIVATE
      uriparser::uriparser
    TEST
      CesiumGltfReader::CesiumGltfReader
)
