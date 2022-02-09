cmake_minimum_required(VERSION 3.15)
project(CesiumJsonWriter CXX)

include(cesium)

cesium_library(
  NAME CesiumJsonWriter
  REQUIRES
    PUBLIC
      CesiumUtility::CesiumUtility
)
