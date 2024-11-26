add_library(Cesium3DTiles "")
configure_cesium_library(Cesium3DTiles)

cesium_glob_files(CESIUM_3DTILES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_3DTILES_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTiles/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/Cesium3DTiles/*.h
)
cesium_glob_files(CESIUM_3DTILES_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_3DTILES_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(Cesium3DTiles
    PROPERTIES
        TEST_SOURCES "${CESIUM_3DTILES_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_TEST_HEADERS}"
)

set_target_properties(Cesium3DTiles
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_PUBLIC_HEADERS}"
)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${CESIUM_3DTILES_SOURCES}
        ${CESIUM_3DTILES_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        Cesium3DTiles
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(Cesium3DTiles
    PUBLIC
        CesiumUtility
)
