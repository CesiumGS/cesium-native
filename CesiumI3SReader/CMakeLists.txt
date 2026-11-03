add_library(CesiumI3SReader "")
configure_cesium_library(CesiumI3SReader)

cesium_glob_files(CESIUM_I3SREADER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUM_I3SREADER_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
)
cesium_glob_files(CESIUM_I3SREADER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumI3SReader/*.h
)

# Tests use CesiumCurl for live-service fetching; omit them when curl is disabled.
if(NOT CESIUM_DISABLE_CURL)
    cesium_glob_files(CESIUM_I3SREADER_TEST_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
    )
    cesium_glob_files(CESIUM_I3SREADER_TEST_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/test/*.h
    )
else()
    set(CESIUM_I3SREADER_TEST_SOURCES "")
    set(CESIUM_I3SREADER_TEST_HEADERS "")
endif()

set_target_properties(CesiumI3SReader
    PROPERTIES
    TEST_SOURCES "${CESIUM_I3SREADER_TEST_SOURCES}"
    TEST_HEADERS "${CESIUM_I3SREADER_TEST_HEADERS}"
)

set_target_properties(CesiumI3SReader
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_I3SREADER_PUBLIC_HEADERS}"
)

target_sources(
    CesiumI3SReader
    PRIVATE
        ${CESIUM_I3SREADER_SOURCES}
        ${CESIUM_I3SREADER_HEADERS}
        ${CESIUM_I3SREADER_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumI3SReader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumI3SReader
    PUBLIC
        CesiumI3S
        CesiumJsonReader
)
