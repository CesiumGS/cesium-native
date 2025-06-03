add_library(CesiumCurl "")

configure_cesium_library(CesiumCurl)

cesium_glob_files(CESIUM_CURL_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_CURL_HEADERS src/*.h)
cesium_glob_files(CESIUM_CURL_PUBLIC_HEADERS include/CesiumCurl/*.h)
cesium_glob_files(CESIUM_CURL_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_CURL_TEST_HEADERS test/*.h)

set_target_properties(CesiumCurl
    PROPERTIES
        TEST_SOURCES "${CESIUM_CURL_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_CURL_TEST_HEADERS}"
)

set_target_properties(CesiumCurl
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_CURL_PUBLIC_HEADERS}"
)

target_sources(
    CesiumCurl
    PRIVATE
        ${CESIUM_CURL_SOURCES}
        ${CESIUM_CURL_HEADERS}
        ${CESIUM_CURL_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumCurl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(CesiumCurl
    PUBLIC
        CesiumAsync
    PRIVATE
        CURL::libcurl
)
