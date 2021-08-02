add_library(CesiumJsonWriter "")

configure_cesium_library(CesiumJsonWriter)

cesium_glob_files(CESIUM_JSON_WRITER_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_JSON_WRITER_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_JSON_WRITER_PUBLIC_HEADERS include/CesiumJsonWriter/*.h)
cesium_glob_files(CESIUM_JSON_WRITER_TEST_SOURCES test/*.cpp test/schema/*.cpp)
cesium_glob_files(CESIUM_JSON_WRITER_TEST_HEADERS test/*.h test/schema/*.h)

set_target_properties(CesiumJsonWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_JSON_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_JSON_WRITER_TEST_HEADERS}"
)

target_sources(
    CesiumJsonWriter
    PRIVATE
        ${CESIUM_JSON_WRITER_SOURCES}
        ${CESIUM_JSON_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_JSON_WRITER_PUBLIC_HEADERS}
)

target_compile_definitions(
    CesiumJsonWriter
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_DEFINES}
)

target_include_directories(
    CesiumJsonWriter
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(CesiumJsonWriter
    PUBLIC
        CesiumUtility
)

install(TARGETS CesiumJsonWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumJsonWriter
)
