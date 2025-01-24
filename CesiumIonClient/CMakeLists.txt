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
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
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

cesium_target_include_directories(
    TARGET
        CesiumIonClient
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(CesiumIonClient
    PUBLIC
        CesiumAsync
        CesiumUtility
        CesiumGeospatial
    PRIVATE
        picosha2::picosha2
        modp_b64::modp_b64
        httplib::httplib
        OpenSSL::Crypto
)
