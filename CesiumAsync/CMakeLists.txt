cmake_minimum_required(VERSION 3.15)
project(CesiumAsync CXX)

include(cesium)

add_library(CesiumAsync "")

configure_cesium_library(CesiumAsync)

cesium_glob_files(CESIUM_ASYNC_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_ASYNC_HEADERS src/*.h)
cesium_glob_files(CESIUM_ASYNC_PUBLIC_HEADERS include/CesiumAsync/*.h)
cesium_glob_files(CESIUM_ASYNC_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_ASYNC_TEST_HEADERS test/*.h)

set_target_properties(CesiumAsync
    PROPERTIES
        TEST_SOURCES "${CESIUM_ASYNC_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ASYNC_TEST_HEADERS}"
)

set_target_properties(CesiumAsync
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_ASYNC_PUBLIC_HEADERS}"
)

target_sources(
    CesiumAsync
    PRIVATE
        ${CESIUM_ASYNC_SOURCES}
        ${CESIUM_ASYNC_HEADERS}
    PUBLIC
        ${CESIUM_ASYNC_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumAsync
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

find_package(Microsoft.GSL REQUIRED)
find_package(Async++ REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(CesiumUtility REQUIRED)

target_link_libraries(CesiumAsync
    PUBLIC
        CesiumUtility::CesiumUtility
        Microsoft.GSL::GSL
        Async++
        spdlog::libspdlog
    PRIVATE
        SQLite::SQLite3
)

install(TARGETS CesiumAsync
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAsync
)

install(
    DIRECTORY include/CesiumAsync/Impl
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAsync
)
