add_library(CesiumGltf "")
configure_cesium_library(CesiumGltf)

if(CESIUM_GLTF_GENERATED_DIRECTORY)
    set(GENERATED_DIRECTORY ${CESIUM_GLTF_GENERATED_DIRECTORY}/generated)
else()
    set(GENERATED_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
endif()

cesium_glob_files(CESIUM_GLTF_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltf/*.cpp
    ${GENERATED_DIRECTORY}/src/CesiumGltf/*.cpp
)
cesium_glob_files(CESIUM_GLTF_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltf/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/CesiumGltf/*.hpp
    ${GENERATED_DIRECTORY}/src/CesiumGltf/*.h
    ${GENERATED_DIRECTORY}/src/CesiumGltf/*.hpp
)
cesium_glob_files(CESIUM_GLTF_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltf/*.h
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumGltf/*.hpp
    ${GENERATED_DIRECTORY}/include/CesiumGltf/*.h
    ${GENERATED_DIRECTORY}/include/CesiumGltf/*.hpp
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
        ${GENERATED_DIRECTORY}/include
    INTERFACE
        # Make private API files accessible to other Cesium Native libraries. This is analagous to protected inheritance in C++  
        # For example, CesiumGltfWriter/AccessorWriter.h (generated) includes CesiumGltf/Accessor.h which includes CesiumGltf/AccessorSpec.h (generated)
        # CesiumGltf/Accessor.h is public API and CesiumGltf/AccessorSpec.h is not, but CesiumGltfWriter still needs to know where to look for CesiumGltf/AccessorSpec.h 

        # TODO: is this going to affect completely external code?
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src
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
