add_library(CesiumVectorData "")

configure_cesium_library(CesiumVectorData)

cesium_glob_files(CESIUM_VECTOR_DATA_SOURCES 
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp)
cesium_glob_files(CESIUM_VECTOR_DATA_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.h
  ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h)
cesium_glob_files(CESIUM_VECTOR_DATA_PUBLIC_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/include/CesiumVectorData/*.h
  ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumVectorData/*.h)
cesium_glob_files(CESIUM_VECTOR_DATA_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_VECTOR_DATA_TEST_HEADERS test/*.h)

set_target_properties(CesiumVectorData
    PROPERTIES
        TEST_SOURCES "${CESIUM_VECTOR_DATA_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_VECTOR_DATA_TEST_HEADERS}"
        TEST_DATA_DIR ${CMAKE_CURRENT_LIST_DIR}/test/data
)

set_target_properties(CesiumVectorData
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_VECTOR_DATA_PUBLIC_HEADERS}"
)

target_sources(
    CesiumVectorData
    PRIVATE
        ${CESIUM_VECTOR_DATA_SOURCES}
        ${CESIUM_VECTOR_DATA_HEADERS}
        ${CESIUM_VECTOR_DATA_PUBLIC_HEADERS}
)

cesium_target_include_directories(
    TARGET
        CesiumVectorData
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

if(NOT IOS AND NOT VCPKG_CMAKE_SYSTEM_NAME MATCHES "iOS")
    set(CESIUM_VECTOR_DATA_ASMJIT_DEPENDENCY asmjit::asmjit)
endif()

target_link_libraries(CesiumVectorData
    PUBLIC
        CesiumUtility
        CesiumGeometry
        CesiumGeospatial
        CesiumAsync
        CesiumGltf
    PRIVATE
        blend2d::blend2d
        ${CESIUM_VECTOR_DATA_ASMJIT_DEPENDENCY}
)
