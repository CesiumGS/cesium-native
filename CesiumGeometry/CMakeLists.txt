cmake_minimum_required(VERSION 3.15)
project(CesiumGeometry CXX)

include(cesium)

cesium_library(
  NAME CesiumGeometry
  REQUIRES
    PUBLIC
      CesiumUtility::CesiumUtility
)
