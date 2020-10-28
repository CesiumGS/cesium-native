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

    # These macro definitions confuse doxygen, causing it
    # to omit the respective elements in the documentation,
    # so ignore these macros during doxygen preprocessing:
    set(DOXYGEN_ENABLE_PREPROCESSING YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set(DOXYGEN_PREDEFINED "CESIUM3DTILES_API" "CESIUMGEOMETRY_API" "CESIUMUTILITY_API" "CESIUMGEOSPATIAL_API")

    doxygen_add_docs(
        cesium-native-docs
        ${LIB_DIRS}
    )
endif()
