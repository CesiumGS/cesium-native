cmake_minimum_required(VERSION 3.15)
project(csprng CXX)

add_library(csprng source/csprng.cpp)
target_include_directories(csprng PUBLIC source)
install(TARGETS csprng DESTINATION lib)
install(
  FILES
    source/duthomhas/csprng.hpp
    source/duthomhas/csprng.h
    source/duthomhas/is_iterable.hpp
  DESTINATION include/duthomhas
)
