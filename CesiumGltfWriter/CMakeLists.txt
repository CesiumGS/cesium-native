add_library(CesiumGltfWriter "")
configure_cesium_library(CesiumGltfWriter)

cesium_glob_files(CESIUM_GLTF_WRITER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_WRITER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_WRITER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfWriter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltfWriter/*.h
)
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_GLTF_WRITER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumGltfWriter
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_WRITER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_WRITER_TEST_HEADERS}"
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

target_compile_definitions(
    CesiumGltfWriter
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_DEFINES}
)

target_include_directories(
    CesiumGltfWriter
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumGltfWriter
    PUBLIC
        CesiumGltf
        CesiumJsonWriter
        modp_b64
)

install(TARGETS CesiumGltfWriter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltfWriter
)
