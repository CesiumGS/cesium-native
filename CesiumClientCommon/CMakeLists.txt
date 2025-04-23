add_library(CesiumClientCommon "")

configure_cesium_library(CesiumClientCommon)

cesium_glob_files(CESIUM_CLIENT_COMMON_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_CLIENT_COMMON_HEADERS src/*.h)
cesium_glob_files(CESIUM_CLIENT_COMMON_PUBLIC_HEADERS include/CesiumClientCommon/*.h)
cesium_glob_files(CESIUM_CLIENT_COMMON_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_CLIENT_COMMON_TEST_HEADERS test/*.h)

set_target_properties(CesiumClientCommon
    PROPERTIES
        TEST_SOURCES "${CESIUM_CLIENT_COMMON_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_CLIENT_COMMON_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumClientCommon
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_CLIENT_COMMON_PUBLIC_HEADERS}"
)

target_sources(
    CesiumClientCommon
    PRIVATE
        ${CESIUM_CLIENT_COMMON_SOURCES}
        ${CESIUM_CLIENT_COMMON_HEADERS}
        ${CESIUM_CLIENT_COMMON_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumClientCommon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(CesiumClientCommon
    PUBLIC
        CesiumAsync
        CesiumUtility
    PRIVATE
        picosha2::picosha2
        modp_b64::modp_b64
        httplib::httplib
        OpenSSL::Crypto
)
