cmake_minimum_required(VERSION 3.15)
project(CesiumGltf CXX)

include(cesium)

cesium_library(
  NAME CesiumGltf
  REQUIRES
    PUBLIC
      Microsoft.GSL::GSL
      CesiumUtility::CesiumUtility
)
