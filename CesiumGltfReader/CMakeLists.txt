add_library(CesiumGltfReader "")
configure_cesium_library(CesiumGltfReader)

cesium_glob_files(CESIUM_GLTF_READER_SOURCES src/*.cpp generated/*.cpp)
cesium_glob_files(CESIUM_GLTF_READER_HEADERS src/*.h src/*.hpp generated/*.h generated/*.hpp)
cesium_glob_files(CESIUM_GLTF_READER_PUBLIC_HEADERS include/CesiumGltf/*.h)
cesium_glob_files(CESIUM_GLTF_READER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GLTF_READER_TEST_HEADERS test/*.h)

set_target_properties(CesiumGltfReader
    PROPERTIES 
        TEST_SOURCES "${CESIUM_GLTF_READER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_READER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

target_sources(
    CesiumGltfReader
    PRIVATE
        ${CESIUM_GLTF_READER_SOURCES}
        ${CESIUM_GLTF_READER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_READER_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltfReader
    SYSTEM PUBLIC
        # Necessary for `draco/draco_features.h`
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR} 
        ${CMAKE_BINARY_DIR}
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(CesiumGltfReader 
    PUBLIC 
        CesiumGltf 
        GSL 
    PRIVATE 
        draco 
        modp_b64
)

install(TARGETS CesiumGltfReader)
install(FILES ${CESIUM_GLTF_READER_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltf)
