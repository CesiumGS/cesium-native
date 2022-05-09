cmake_minimum_required(VERSION 3.15)
project(CesiumGeospatial CXX)

include(cesium)

cesium_library(
  NAME CesiumGeospatial
  REQUIRES
    PUBLIC
      CesiumGeometry::CesiumGeometry
      CesiumUtility::CesiumUtility
    PRIVATE
      earcut::earcut
      s2geometry::s2geometry
)
