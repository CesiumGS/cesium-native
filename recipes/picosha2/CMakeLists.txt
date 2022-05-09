cmake_minimum_required(VERSION 3.15)
project(picosha2 CXX)

add_library(picosha2 INTERFACE)
install(
  FILES
    picosha2.h
  DESTINATION include
)
