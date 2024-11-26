add_library(CesiumGltfReader "")
configure_cesium_library(CesiumGltfReader)

cesium_glob_files(CESIUM_GLTF_READER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_READER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_READER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltfReader/*.h
)
cesium_glob_files(CESIUM_GLTF_READER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_GLTF_READER_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumGltfReader
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_READER_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_READER_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumGltfReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_READER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltfReader
    PRIVATE
        ${CESIUM_GLTF_READER_SOURCES}
        ${CESIUM_GLTF_READER_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_READER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumGltfReader
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumGltfReader
    PUBLIC
        CesiumAsync
        CesiumGltf
        CesiumJsonReader
        draco::draco
    PRIVATE
        meshoptimizer::meshoptimizer
        modp_b64::modp_b64
        KTX::ktx
        WebP::webp WebP::webpdecoder
        $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
)
