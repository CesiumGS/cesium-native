add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

cesium_glob_files(CESIUM_GEOMETRY_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GEOMETRY_HEADERS src/*.h)
cesium_glob_files(CESIUM_GEOMETRY_PUBLIC_HEADERS include/CesiumGeometry/*.h)
cesium_glob_files(CESIUM_GEOMETRY_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GEOMETRY_TEST_HEADERS test/*.h)

set_target_properties(CesiumGeometry
    PROPERTIES
        TEST_SOURCES "${CESIUM_GEOMETRY_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GEOMETRY_TEST_HEADERS}"
)

set_target_properties(CesiumGeometry
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GEOMETRY_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGeometry
    PRIVATE
        ${CESIUM_GEOMETRY_SOURCES}
        ${CESIUM_GEOMETRY_HEADERS}
    PUBLIC
        ${CESIUM_GEOMETRY_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeometry
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(CesiumGeometry PUBLIC
    CesiumUtility
)

install(TARGETS CesiumGeometry
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGeometry
)
