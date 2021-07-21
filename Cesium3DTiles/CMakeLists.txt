add_library(Cesium3DTiles INTERFACE)

cesium_glob_files(CESIUM_3DTILES_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_3DTILES_PUBLIC_HEADERS include/Cesium3DTiles/*.h)

set_target_properties(Cesium3DTiles
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_3DTILES_PUBLIC_HEADERS}"
)

target_include_directories(
    Cesium3DTiles
    SYSTEM INTERFACE
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include/
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

install(TARGETS Cesium3DTiles
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Cesium3DTiles
)
