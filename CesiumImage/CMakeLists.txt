add_library(CesiumImage "")
configure_cesium_library(CesiumImage)

cesium_glob_files(CESIUM_IMAGE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
cesium_glob_files(CESIUM_IMAGE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
cesium_glob_files(CESIUM_IMAGE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumImage/*.h
)
cesium_glob_files(CESIUM_IMAGE_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_IMAGE_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumImage
    PROPERTIES
        TEST_SOURCES "${CESIUM_IMAGE_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_IMAGE_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumImage
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_IMAGE_PUBLIC_HEADERS}"
)

target_sources(
    CesiumImage
    PRIVATE
        ${CESIUM_IMAGE_SOURCES}
        ${CESIUM_IMAGE_HEADERS}
        ${CESIUM_IMAGE_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumImage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(CesiumImage
    PUBLIC
        CesiumUtility
    PRIVATE
        KTX::ktx
        WebP::webp
        WebP::webpdecoder
)

if(CESIUM_DISABLE_LIBJPEG_TURBO)
    target_compile_definitions(
        CesiumImage
        PRIVATE
            CESIUM_DISABLE_LIBJPEG_TURBO
    )
else()
    target_link_libraries(
        CesiumImage
        PRIVATE
            $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    )
endif()
