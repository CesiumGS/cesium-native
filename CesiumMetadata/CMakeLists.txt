add_library(CesiumMetadata "")

configure_cesium_library(CesiumMetadata)

cesium_glob_files(CESIUM_METADATA_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_METADATA_HEADERS src/*.h)
cesium_glob_files(CESIUM_METADATA_PUBLIC_HEADERS include/CesiumMetadata/*.h)
cesium_glob_files(CESIUM_METADATA_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_METADATA_TEST_HEADERS test/*.h)

set_target_properties(CesiumMetadata
    PROPERTIES
        TEST_SOURCES "${CESIUM_METADATA_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_METADATA_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumMetadata
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_METADATA_PUBLIC_HEADERS}"
)

target_sources(
    CesiumMetadata
    PRIVATE
        ${CESIUM_METADATA_SOURCES}
        ${CESIUM_METADATA_HEADERS}
        ${CESIUM_METADATA_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumMetadata
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(
    CesiumMetadata
    PUBLIC
        Cesium3DTiles
        CesiumGltf
        CesiumUtility
        glm::glm
)
