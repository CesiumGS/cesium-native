add_library(CesiumMetadata "")

configure_cesium_library(CesiumMetadata)

cesium_glob_files(CESIUM_METADATA_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_METADATA_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_METADATA_PUBLIC_HEADERS include/CesiumUtility/*.h)
cesium_glob_files(CESIUM_METADATA_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_METADATA_TEST_HEADERS test/*.h)

set_target_properties(CesiumMetadata
    PROPERTIES
        TEST_SOURCES "${CESIUM_METADATA_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_METADATA_TEST_HEADERS}"
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
    PUBLIC
        ${CESIUM_METADATA_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumUtility
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumMetadata
    PUBLIC
        GSL
)

install(TARGETS CesiumMetadata
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumMetadata
)