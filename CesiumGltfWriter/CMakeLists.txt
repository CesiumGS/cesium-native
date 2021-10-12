add_library(CesiumGltfWriter "")
configure_cesium_library(CesiumGltfWriter)

cesium_glob_files(CESIUM_GLTF_WRITER_SOURCES src/*.cpp generated/*.cpp)
cesium_glob_files(CESIUM_GLTF_WRITER_HEADERS src/*.h src/*.hpp generated/*.h generated/*.hpp)
cesium_glob_files(CESIUM_GLTF_WRITER_PUBLIC_HEADERS include/CesiumGltf/*.h) # Correct is CesiumGltf not CesiumGltfWriter
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_HEADERS test/*.h)

set_target_properties(CesiumGltfWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_WRITER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumGltfWriter
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_WRITER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_GLTF_WRITER_SOURCES}
        ${CESIUM_GLTF_WRITER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_WRITER_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltfWriter
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

target_link_libraries(CesiumGltfWriter
    PUBLIC
        CesiumGltf
        CesiumJsonWriter
        GSL
        modp_b64
        ${CESIUM_NATIVE_DRACO_LIBRARY}
)

install(TARGETS CesiumGltfWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltf # Correct is CesiumGltf not CesiumGltfWriter
)
