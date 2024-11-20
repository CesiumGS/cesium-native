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

cesium_target_include_directories(
    TARGET 
        CesiumAsync
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(CesiumAsync
    PUBLIC
        CesiumUtility
        spdlog::spdlog spdlog::spdlog_header_only
        Async++
    PRIVATE
        unofficial::sqlite3::sqlite3
)


if (PRIVATE_CESIUM_SQLITE)
  target_compile_definitions(CesiumAsync PUBLIC PRIVATE_CESIUM_SQLITE)
endif()
