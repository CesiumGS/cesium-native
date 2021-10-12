add_library(Cesium3DTilesWriter "")
configure_cesium_library(Cesium3DTilesWriter)

cesium_glob_files(CESIUM_3DTILES_WRITER_SOURCES src/*.cpp generated/*.cpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_HEADERS src/*.h src/*.hpp generated/*.h generated/*.hpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_PUBLIC_HEADERS include/Cesium3DTiles/*.h) # Correct is Cesium3DTiles not Cesium3DTilesWriter
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_HEADERS test/*.h)

set_target_properties(Cesium3DTilesWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_3DTILES_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_WRITER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(Cesium3DTilesWriter
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_WRITER_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTilesWriter
    PRIVATE
        ${CESIUM_3DTILES_WRITER_SOURCES}
        ${CESIUM_3DTILES_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_WRITER_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTilesWriter
    SYSTEM PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(Cesium3DTilesWriter
    PUBLIC
        Cesium3DTiles
        CesiumJsonWriter
        GSL
)

install(TARGETS Cesium3DTilesWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTiles # Correct is Cesium3DTiles not Cesium3DTilesWriter
)
