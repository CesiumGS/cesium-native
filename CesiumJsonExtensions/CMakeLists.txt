add_library(CesiumJsonExtensions "")
configure_cesium_library(CesiumJsonExtensions)

cesium_glob_files(CESIUM_JSON_EXTENSIONS_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_JSON_EXTENSIONS_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_JSON_EXTENSIONS_PUBLIC_HEADERS include/CesiumJsonExtensions/*.h)
cesium_glob_files(CESIUM_JSON_EXTENSIONS_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_JSON_EXTENSIONS_TEST_HEADERS test/*.h)

set_target_properties(CesiumJsonExtensions
    PROPERTIES
        TEST_SOURCES "${CESIUM_JSON_EXTENSIONS_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_JSON_EXTENSIONS_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumJsonExtensions
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_JSON_EXTENSIONS_PUBLIC_HEADERS}"
)

target_sources(
    CesiumJsonExtensions
    PRIVATE
        ${CESIUM_JSON_EXTENSIONS_SOURCES}
        ${CESIUM_JSON_EXTENSIONS_HEADERS}
    PUBLIC
        ${CESIUM_JSON_EXTENSIONS_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumJsonExtensions
    SYSTEM PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumJsonExtensions
    PUBLIC
      CesiumJsonReader
      CesiumUtility
)

install(TARGETS CesiumJsonExtensions
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumJsonExtensions
)
