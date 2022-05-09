cmake_minimum_required(VERSION 3.15)
project(CesiumUtility CXX)

include(cesium)

cesium_library(
  NAME CesiumUtility
  REQUIRES
    PUBLIC
      Microsoft.GSL::GSL
      glm::glm
      rapidjson::rapidjson
    PRIVATE
      uriparser::uriparser
)
