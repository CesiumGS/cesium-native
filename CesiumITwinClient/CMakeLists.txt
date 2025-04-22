add_library(CesiumITwinClient "")

configure_cesium_library(CesiumITwinClient)

cesium_glob_files(CESIUM_ITWIN_CLIENT_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_ITWIN_CLIENT_HEADERS src/*.h)
cesium_glob_files(CESIUM_ITWIN_CLIENT_PUBLIC_HEADERS include/CesiumITwinClient/*.h)
cesium_glob_files(CESIUM_ITWIN_CLIENT_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_ITWIN_CLIENT_TEST_HEADERS test/*.h)

set_target_properties(CesiumITwinClient
    PROPERTIES
        TEST_SOURCES "${CESIUM_ITWIN_CLIENT_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ITWIN_CLIENT_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumITwinClient
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_ITWIN_CLIENT_PUBLIC_HEADERS}"
)

target_sources(
    CesiumITwinClient
    PRIVATE
        ${CESIUM_ITWIN_CLIENT_SOURCES}
        ${CESIUM_ITWIN_CLIENT_HEADERS}
        ${CESIUM_ITWIN_CLIENT_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumITwinClient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(CesiumITwinClient
    PUBLIC
        CesiumAsync
        CesiumUtility
        CesiumClientCommon
        CesiumGeospatial
    PRIVATE
        modp_b64::modp_b64
)
