add_library(CesiumAnalysis "")
configure_cesium_library(CesiumAnalysis)

cesium_glob_files(CESIUM_ANALYSIS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.cpp
)
cesium_glob_files(CESIUM_ANALYSIS_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.h
)
cesium_glob_files(CESIUM_ANALYSIS_PUBLIC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/CesiumAnalysis/*.h
    ${CMAKE_CURRENT_LIST_DIR}/generated/include/CesiumAnalysis/*.h
)
cesium_glob_files(CESIUM_ANALYSIS_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test/*.cpp
)
cesium_glob_files(CESIUM_ANALYSIS_TEST_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/test/*.h
)

set_target_properties(CesiumAnalysis
    PROPERTIES
        TEST_SOURCES "${CESIUM_ANALYSIS_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ANALYSIS_TEST_HEADERS}"
)

set_target_properties(CesiumAnalysis
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_ANALYSIS_PUBLIC_HEADERS}"
)

target_sources(
    CesiumAnalysis
    PRIVATE
        ${CESIUM_ANALYSIS_SOURCES}
        ${CESIUM_ANALYSIS_HEADERS}
    PUBLIC
        ${CESIUM_ANALYSIS_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumAnalysis
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/generated/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/generated/src
        ${CMAKE_CURRENT_LIST_DIR}/../extern/PhysX/physx/include
)

# Total hack here!
# PhysX must be built manually before building this library. And to do that, you must edit:
#  - extern/PhysX/physx/buildtools/presets/public/vc17win64.xml
# And change the values of `PX_GENERATE_STATIC_LIBRARIES` to `True` and `NV_USE_STATIC_WINCRT` to `False`.

target_link_directories(CesiumAnalysis PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../extern/PhysX/physx/bin/win.x86_64.vc143.md/debug")
target_link_libraries(CesiumAnalysis
    PUBLIC
        CesiumUtility
        CesiumGltf
        Cesium3DTiles
        PhysXExtensions_static_64
        PhysX_static_64
        PhysXCooking_static_64
        PhysXCommon_static_64
        PhysXFoundation_static_64
        PhysXPvdSDK_static_64
)

install(TARGETS CesiumAnalysis
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAnalysis
)
