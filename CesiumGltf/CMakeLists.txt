add_library(CesiumGltf "")
configure_cesium_library(CesiumGltf)

cesium_glob_files(CESIUM_GLTF_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_GLTF_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_GLTF_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltf/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumGltf/*.h
)
cesium_glob_files(CESIUM_GLTF_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_GLTF_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumGltf
    PROPERTIES
        TEST_SOURCES "${CESIUM_GLTF_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GLTF_TEST_HEADERS}"
)

set_target_properties(CesiumGltf
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GLTF_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGltf
    PRIVATE
        ${CESIUM_GLTF_SOURCES}
        ${CESIUM_GLTF_HEADERS}
    PUBLIC
        ${CESIUM_GLTF_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGltf
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
)

target_link_libraries(CesiumGltf
    PUBLIC
        CesiumUtility
)
