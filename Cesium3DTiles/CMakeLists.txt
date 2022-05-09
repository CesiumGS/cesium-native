cmake_minimum_required(VERSION 3.15)
project(Cesium3DTiles CXX)

include(cesium)

cesium_library(
  NAME Cesium3DTiles
  REQUIRES
    PUBLIC
      Microsoft.GSL::GSL
      CesiumUtility::CesiumUtility
)
