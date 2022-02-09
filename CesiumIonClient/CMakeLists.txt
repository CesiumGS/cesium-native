cmake_minimum_required(VERSION 3.15)
project(CesiumIonClient CXX)

include(cesium)

cesium_library(
  NAME CesiumIonClient
  REQUIRES
    PUBLIC
      CesiumAsync::CesiumAsync
      CesiumUtility::CesiumUtility
    PRIVATE
      csprng::csprng
      base64::base64
      picosha2::picosha2
      httplib::httplib
)
