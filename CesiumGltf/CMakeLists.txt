add_library(CesiumGltf "")
configure_cesium_library(CesiumGltf)

cesium_glob_files(CESIUM_GLTF_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GLTF_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_GLTF_PUBLIC_HEADERS include/CesiumGltf/*.h)
cesium_glob_files(CESIUM_GLTF_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GLTF_TEST_HEADERS test/*.h)

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
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include/
        ${CESIUM_NATIVE_GSL_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumGltf
    PUBLIC
        CesiumUtility
        GSL
)

install(TARGETS CesiumGltf
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGltf
)
