add_library(Cesium3DTilesWriter "")
configure_cesium_library(Cesium3DTilesWriter)

if(CESIUM_3DTILES_WRITER_GENERATED_DIRECTORY)
    set(GENERATED_DIRECTORY ${CESIUM_3DTILES_WRITER_GENERATED_DIRECTORY}/generated)
else()
    set(GENERATED_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
endif()

cesium_glob_files(CESIUM_3DTILES_WRITER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTilesWriter/*.cpp
    ${GENERATED_DIRECTORY}/src/Cesium3DTilesWriter/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_WRITER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTilesWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTilesWriter/*.hpp
    ${GENERATED_DIRECTORY}/src/Cesium3DTilesWriter/*.h
    ${GENERATED_DIRECTORY}/src/Cesium3DTilesWriter/*.hpp
)
cesium_glob_files(CESIUM_3DTILES_WRITER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTilesWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTilesWriter/*.hpp
    ${GENERATED_DIRECTORY}/include/Cesium3DTilesWriter/*.h
    ${GENERATED_DIRECTORY}/include/Cesium3DTilesWriter/*.hpp
)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_WRITER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

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
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${GENERATED_DIRECTORY}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src
)

target_link_libraries(Cesium3DTilesWriter
    PUBLIC
        Cesium3DTiles
        CesiumJsonWriter
        GSL
)

install(TARGETS Cesium3DTilesWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTilesWriter
)
