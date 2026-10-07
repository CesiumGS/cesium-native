add_library(CesiumI3SContent "")
configure_cesium_library(CesiumI3SContent)

cesium_glob_files(CESIUMI3SCONTENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUMI3SCONTENT_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
)
cesium_glob_files(CESIUMI3SCONTENT_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumI3SContent/*.h
)
cesium_glob_files(CESIUMI3SCONTENT_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUMI3SCONTENT_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumI3SContent
    PROPERTIES
        TEST_SOURCES "${CESIUMI3SCONTENT_TEST_SOURCES}"
        TEST_HEADERS "${CESIUMI3SCONTENT_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumI3SContent
    PROPERTIES
    PUBLIC_HEADER "${CESIUMI3SCONTENT_PUBLIC_HEADERS}"
)

target_sources(
    CesiumI3SContent
    PRIVATE
        ${CESIUMI3SCONTENT_SOURCES}
        ${CESIUMI3SCONTENT_HEADERS}
        ${CESIUMI3SCONTENT_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumI3SContent
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumI3SContent
    PUBLIC
        CesiumAsync
        CesiumGeometry
        CesiumGeospatial
        CesiumGltf
        CesiumI3S
        CesiumI3SReader
        CesiumUtility
    PRIVATE
        draco::draco
)
