find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(
        LIB_DIRS
        ../Cesium3DTiles/include
        ../Cesium3DTiles/generated/include
        ../Cesium3DTilesReader/include
        ../Cesium3DTilesWriter/include
        ../Cesium3DTilesSelection/include
        ../CesiumAsync/include
        ../CesiumGeometry/include
        ../CesiumGeospatial/include
        ../CesiumGltf/include
        ../CesiumGltf/generated/include
        ../CesiumGltfReader/include
        ../CesiumGltfWriter/include
        ../CesiumIonClient/include
        ../CesiumJsonReader/include
        ../CesiumJsonWriter/include
        ../CesiumQuantizedMeshTerrain/include
        ../CesiumRasterOverlays/include
        ../CesiumUtility/include
    )

    set(DOXYGEN_STRIP_FROM_INC_PATH ${LIB_DIRS})
    set(
        DOXYGEN_EXAMPLE_PATH
        ../Cesium3DTiles/test
        ../Cesium3DTilesReader/test
        ../Cesium3DTilesWriter/test
        ../Cesium3DTilesSelection/test
        ../CesiumAsync/test
        ../CesiumGeometry/test
        ../CesiumGeospatial/test
        ../CesiumGltf/test
        ../CesiumGltfReader/test
        ../CesiumGltfWriter/test
        ../CesiumIonClient/test
        ../CesiumJsonReader/test
        ../CesiumJsonWriter/test
        ../CesiumQuantizedMeshTerrain/test
        ../CesiumRasterOverlays/test
        ../CesiumUtility/test
    )

    # These macro definitions confuse doxygen, causing it
    # to omit the respective elements in the documentation,
    # so ignore these macros during doxygen preprocessing:
    set(DOXYGEN_ENABLE_PREPROCESSING YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set(DOXYGEN_PREDEFINED "CESIUM3DTILESSELECTION_API" "CESIUMGEOMETRY_API" "CESIUMUTILITY_API" "CESIUMGEOSPATIAL_API")
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_CURRENT_LIST_DIR}/../node_modules/doxygen-awesome-css/doxygen-awesome.css")
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "../README.md")
    set(DOXYGEN_EXCLUDE_PATTERNS "*/node_modules/*")
    set(DOXYGEN_IMAGE_PATH "./")

    doxygen_add_docs(
        cesium-native-docs
        "../README.md"
        ${LIB_DIRS}
    )
endif()
