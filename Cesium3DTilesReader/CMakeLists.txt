cmake_minimum_required(VERSION 3.15)
project(Cesium3DTilesReader CXX)

include(cesium)

cesium_library(
  NAME Cesium3DTilesReader
  REQUIRES
    PUBLIC
      Microsoft.GSL::GSL
      Cesium3DTiles::Cesium3DTiles
      CesiumJsonReader::CesiumJsonReader
)
