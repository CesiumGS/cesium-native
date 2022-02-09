cmake_minimum_required(VERSION 3.15)
project(Cesium3DTilesWriter CXX)

include(cesium)

cesium_library(
  NAME Cesium3DTilesWriter
  REQUIRES
    PUBLIC
      Microsoft.GSL::GSL
      Cesium3DTiles::Cesium3DTiles
      CesiumJsonWriter::CesiumJsonWriter
    TEST
      Cesium3DTilesReader::Cesium3DTilesReader
)
