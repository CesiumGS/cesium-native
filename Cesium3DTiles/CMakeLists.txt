add_library(Cesium3DTiles "")
configure_cesium_library(Cesium3DTiles)

if(CESIUM_3D_TILES_GENERATED_DIRECTORY)
    set(GENERATED_DIRECTORY ${CESIUM_3D_TILES_GENERATED_DIRECTORY}/generated)
else()
    set(GENERATED_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
endif()

cesium_glob_files(CESIUM_3D_TILES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTiles/*.cpp
    ${GENERATED_DIRECTORY}/src/Cesium3DTiles/*.cpp
)
cesium_glob_files(CESIUM_3D_TILES_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTiles/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/Cesium3DTiles/*.hpp
    ${GENERATED_DIRECTORY}/src/Cesium3DTiles/*.h
    ${GENERATED_DIRECTORY}/src/Cesium3DTiles/*.hpp
)
cesium_glob_files(CESIUM_3DTILES_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTiles/*.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Cesium3DTiles/*.hpp
    ${GENERATED_DIRECTORY}/include/Cesium3DTiles/*.h
    ${GENERATED_DIRECTORY}/include/Cesium3DTiles/*.hpp
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

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${GENERATED_DIRECTORY}/include
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src    
    PRIVATE
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${GENERATED_DIRECTORY}/src
)

target_link_libraries(Cesium3DTiles
    PUBLIC
        CesiumUtility
        GSL
)

install(TARGETS Cesium3DTiles
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTiles
)
