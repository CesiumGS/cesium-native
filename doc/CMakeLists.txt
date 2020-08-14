find_package(Doxygen)

if (DOXYGEN_FOUND)
    set(
        LIB_DIRS
        ../Cesium3DTiles/include
        ../CesiumGeospatial/include
        ../CesiumGeometry/include
        ../CesiumUtility/include
    )

    set(DOXYGEN_STRIP_FROM_INC_PATH ${LIB_DIRS})
    set(
        DOXYGEN_EXAMPLE_PATH
        ../Cesium3DTiles/test
        ../CesiumGeospatial/test
        ../CesiumGeometry/test
        ../CesiumUtility/test
    )

    doxygen_add_docs(
        cesium-native-docs
        ${LIB_DIRS}
    )
endif()
