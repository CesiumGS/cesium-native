cmake_minimum_required(VERSION 3.15)
project(CesiumIonClient CXX)

include(cesium)

add_library(CesiumIonClient "")

configure_cesium_library(CesiumIonClient)

cesium_glob_files(CESIUM_ION_CLIENT_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_ION_CLIENT_HEADERS src/*.h)
cesium_glob_files(CESIUM_ION_CLIENT_PUBLIC_HEADERS include/CesiumIonClient/*.h)
cesium_glob_files(CESIUM_ION_CLIENT_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_ION_CLIENT_TEST_HEADERS test/*.h)

set_target_properties(CesiumIonClient
    PROPERTIES
        TEST_SOURCES "${CESIUM_ION_CLIENT_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ION_CLIENT_TEST_HEADERS}"
)

set_target_properties(CesiumIonClient
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_ION_CLIENT_PUBLIC_HEADERS}"
)

target_sources(
    CesiumIonClient
    PRIVATE
        ${CESIUM_ION_CLIENT_SOURCES}
        ${CESIUM_ION_CLIENT_HEADERS}
    PUBLIC
        ${CESIUM_ION_CLIENT_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumIonClient
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
add_library(csprng ../extern/CSPRNG/source/csprng.cpp)
target_include_directories(
    csprng
    PUBLIC
        ../extern/CSPRNG/source
)
set(CSPRNG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.h ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.hpp ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/is_iterable.hpp)

# PicoSHA2 doesn't have CMakeLists.txt at all
add_library(PicoSHA2 INTERFACE)
target_include_directories(PicoSHA2 SYSTEM INTERFACE ../extern/PicoSHA2)
set_target_properties(PicoSHA2 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../extern/PicoSHA2/picosha2.h")

find_package(base64 REQUIRED)
find_package(httplib REQUIRED)
find_package(CesiumAsync REQUIRED)
find_package(CesiumUtility REQUIRED)

target_link_libraries(CesiumIonClient
    PUBLIC
        CesiumAsync::CesiumAsync
        CesiumUtility::CesiumUtility
    PRIVATE
        csprng
        base64::base64
        PicoSHA2
        httplib::httplib
)

# httplib erroneously does not declare its include a `SYSTEM` include, so
# we must extract its include_directories using this function and manually
# reinclude it as 'SYSTEM'.
target_link_libraries_system(CesiumIonClient
    PRIVATE
        httplib::httplib
)

install(TARGETS CesiumIonClient
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumIonClient
)
