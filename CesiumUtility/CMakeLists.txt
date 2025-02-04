add_library(CesiumUtility "")

configure_cesium_library(CesiumUtility)

cesium_glob_files(CESIUM_UTILITY_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_UTILITY_HEADERS src/*.h)
cesium_glob_files(CESIUM_UTILITY_PUBLIC_HEADERS include/CesiumUtility/*.h)
cesium_glob_files(CESIUM_UTILITY_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_UTILITY_TEST_HEADERS test/*.h)

set_target_properties(CesiumUtility
    PROPERTIES
        TEST_SOURCES "${CESIUM_UTILITY_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_UTILITY_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumUtility
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_UTILITY_PUBLIC_HEADERS}"
)

target_sources(
    CesiumUtility
    PRIVATE
        ${CESIUM_UTILITY_SOURCES}
        ${CESIUM_UTILITY_HEADERS}
    PUBLIC
        ${CESIUM_UTILITY_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumUtility
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/../extern/zlib
        ${CMAKE_CURRENT_BINARY_DIR}/../extern/zlib-src
)

target_link_libraries(
    CesiumUtility
    PUBLIC
        zlib-ng::zlib-ng
        spdlog::spdlog
        glm::glm
        ada::ada
)
