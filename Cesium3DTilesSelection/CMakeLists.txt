cmake_minimum_required(VERSION 3.15)
project(Cesium3DTilesSelection CXX)

include(cesium)

cesium_library(
  NAME Cesium3DTilesSelection
  REQUIRES
    PUBLIC
      spdlog::spdlog
      CesiumAsync::CesiumAsync
      CesiumGeospatial::CesiumGeospatial
      CesiumGeometry::CesiumGeometry
      CesiumGltf::CesiumGltf
      CesiumGltfReader::CesiumGltfReader
      CesiumUtility::CesiumUtility
    PRIVATE
      tinyxml2::tinyxml2
      uriparser::uriparser
)
