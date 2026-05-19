add_library(CesiumI3SSelection "")
configure_cesium_library(CesiumI3SSelection)

cesium_glob_files(CESIUMI3SSELECTION_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUMI3SSELECTION_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
)
cesium_glob_files(CESIUMI3SSELECTION_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumI3SSelection/*.h
)
cesium_glob_files(CESIUMI3SSELECTION_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUMI3SSELECTION_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumI3SSelection
    PROPERTIES
        TEST_SOURCES "${CESIUMI3SSELECTION_TEST_SOURCES}"
        TEST_HEADERS "${CESIUMI3SSELECTION_TEST_HEADERS}"
        PUBLIC_HEADER "${CESIUMI3SSELECTION_PUBLIC_HEADERS}"
)

target_sources(
    CesiumI3SSelection
    PRIVATE
        ${CESIUMI3SSELECTION_SOURCES}
        ${CESIUMI3SSELECTION_HEADERS}
        ${CESIUMI3SSELECTION_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumI3SSelection
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumI3SSelection
    PUBLIC
        Cesium3DTilesSelection
        CesiumAsync
        CesiumGeospatial
        CesiumI3S
        CesiumUtility
    PRIVATE
        CesiumI3SContent
        CesiumI3SReader
        glm::glm
        spdlog::spdlog
)
