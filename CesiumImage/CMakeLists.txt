add_library(CesiumImage "")
configure_cesium_library(CesiumImage)

cesium_glob_files(CESIUM_IMAGE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUM_IMAGE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
cesium_glob_files(CESIUM_IMAGE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumImage/*.h
)
cesium_glob_files(CESIUM_IMAGE_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_IMAGE_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumImage
    PROPERTIES
        TEST_SOURCES "${CESIUM_IMAGE_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_IMAGE_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumImage
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_IMAGE_PUBLIC_HEADERS}"
)

target_sources(
    CesiumImage
    PRIVATE
        ${CESIUM_IMAGE_SOURCES}
        ${CESIUM_IMAGE_HEADERS}
        ${CESIUM_IMAGE_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumImage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumImage
    PUBLIC
        CesiumUtility
    PRIVATE
        KTX::ktx
)
