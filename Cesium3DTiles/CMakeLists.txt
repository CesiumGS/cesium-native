add_library(Cesium3DTiles "")

configure_cesium_library(Cesium3DTiles)

cesium_glob_files(CESIUM_3DTILES_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_3DTILES_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_3DTILES_PUBLIC_HEADERS include/Cesium3DTiles/*.h include/Cesium3DTiles/*.hpp)
cesium_glob_files(CESIUM_3DTILES_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_3DTILES_TEST_HEADERS test/*.h test/*.hpp)

set_target_properties(Cesium3DTiles 
    PROPERTIES 
        TEST_SOURCES "${CESIUM_3DTILES_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_3DTILES_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${CESIUM_3DTILES_SOURCES}
        ${CESIUM_3DTILES_HEADERS}
    PUBLIC
        ${CESIUM_3DTILES_PUBLIC_HEADERS}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(Cesium3DTiles 
    PUBLIC 
        CesiumAsync 
        CesiumGeospatial 
        CesiumGeometry 
        CesiumGltf 
        CesiumGltfReader 
        CesiumUtility
        spdlog
    # PRIVATE
        tinyxml2 
        uriparser 
)

install(TARGETS Cesium3DTiles)
install(FILES ${CESIUM_3DTILES_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTiles)
