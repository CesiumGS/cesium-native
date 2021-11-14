add_library(CesiumGltfReader "")
configure_cesium_library(CesiumGltfReader)

if(CESIUM_GLTF_READER_GENERATED_DIRECTORY)
    set(GENERATED_DIRECTORY ${CESIUM_GLTF_READER_GENERATED_DIRECTORY}/generated)
else()
    set(GENERATED_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
endif()

cesium_glob_files(CESIUM_GLTF_READER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltfReader/*.cpp
    ${GENERATED_DIRECTORY}/src/CesiumGltfReader/*.cpp
)
cesium_glob_files(CESIUM_GLTF_READER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltfReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltfReader/*.hpp
    ${GENERATED_DIRECTORY}/src/CesiumGltfReader/*.h
    ${GENERATED_DIRECTORY}/src/CesiumGltfReader/*.hpp
)
cesium_glob_files(CESIUM_GLTF_READER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfReader/*.h
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltfReader/*.hpp
    ${GENERATED_DIRECTORY}/include/CesiumGltfReader/*.h
    ${GENERATED_DIRECTORY}/include/CesiumGltfReader/*.hpp
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

target_include_directories(
    CesiumGltfReader
    SYSTEM PUBLIC
        # Necessary for `draco/draco_features.h`
        ${CESIUM_NATIVE_DRACO_INCLUDE_DIR}
        ${CESIUM_NATIVE_STB_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${GENERATED_DIRECTORY}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src
)

target_link_libraries(CesiumGltfReader
    PUBLIC
        CesiumGltf
        CesiumJsonReader
        modp_b64
        ${CESIUM_NATIVE_DRACO_LIBRARY}
)

install(TARGETS CesiumGltfReader
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltfReader
)
