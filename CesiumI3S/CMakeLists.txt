add_library(CesiumI3S "")
configure_cesium_library(CesiumI3S)

cesium_glob_files(CESIUM_I3S_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUM_I3S_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumI3S/*.h
)

set_target_properties(CesiumI3S
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_I3S_PUBLIC_HEADERS}"
    TEST_SOURCES ""
    TEST_HEADERS ""
)

target_sources(
    CesiumI3S
    PRIVATE
        ${CESIUM_I3S_SOURCES}
        ${CESIUM_I3S_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumI3S
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)


target_link_libraries(CesiumI3S
    PUBLIC
        CesiumUtility
)
