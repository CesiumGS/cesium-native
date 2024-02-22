add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

cesium_glob_files(CESIUM_GEOSPATIAL_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_GEOSPATIAL_HEADERS src/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_PUBLIC_HEADERS include/CesiumGeospatial/*.h)
cesium_glob_files(CESIUM_GEOSPATIAL_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_GEOSPATIAL_TEST_HEADERS test/*.h)

set_target_properties(CesiumGeospatial
    PROPERTIES
        TEST_SOURCES "${CESIUM_GEOSPATIAL_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_GEOSPATIAL_TEST_HEADERS}"
)

set_target_properties(CesiumGeospatial
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_GEOSPATIAL_PUBLIC_HEADERS}"
)

target_sources(
    CesiumGeospatial
    PRIVATE
        ${CESIUM_GEOSPATIAL_SOURCES}
        ${CESIUM_GEOSPATIAL_HEADERS}
    PUBLIC
        ${CESIUM_GEOSPATIAL_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeospatial
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(CesiumGeospatial PUBLIC
    CesiumGeometry
    CesiumUtility
    earcut
    PRIVATE
      s2geometry
)

install(TARGETS CesiumGeospatial
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumGeospatial
)
