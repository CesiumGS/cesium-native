cmake_minimum_required(VERSION 3.15)
project(CesiumAsync CXX)

include(cesium)

cesium_library(
  NAME CesiumAsync
  REQUIRES
    PUBLIC
      CesiumUtility::CesiumUtility
      Microsoft.GSL::GSL
      Async++
      spdlog::libspdlog
    PRIVATE
      SQLite::SQLite3@SQLite3
)
