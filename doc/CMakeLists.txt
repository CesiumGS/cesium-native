find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(
        LIB_DIRS
        ../Cesium3DTiles/include
        ../Cesium3DTiles/generated/include
        ../Cesium3DTilesContent/include
        ../Cesium3DTilesReader/include
        ../Cesium3DTilesReader/generated/include
        ../Cesium3DTilesWriter/include
        ../Cesium3DTilesSelection/include
        ../CesiumAsync/include
        ../CesiumGeometry/include
        ../CesiumGeospatial/include
        ../CesiumGltf/include
        ../CesiumGltf/generated/include
        ../CesiumGltfContent/include
        ../CesiumGltfReader/include
        ../CesiumGltfReader/generated/include
        ../CesiumGltfWriter/include
        ../CesiumIonClient/include
        ../CesiumJsonReader/include
        ../CesiumJsonWriter/include
        ../CesiumQuantizedMeshTerrain/include
        ../CesiumQuantizedMeshTerrain/generated/include
        ../CesiumRasterOverlays/include
        ../CesiumUtility/include
    )

    set(DOXYGEN_STRIP_FROM_INC_PATH ${LIB_DIRS})

    # Instead of specifying the full source tree in the example path, specify only the Cesium* lib directories to reduce parsing time
    file(GLOB example_dirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_LIST_DIR} "../Cesium*")
    foreach(child ${example_dirs})
      if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${child})
        list(APPEND DOXYGEN_EXAMPLE_PATH ${CMAKE_CURRENT_LIST_DIR}/${child})
      endif()
    endforeach()

    # These macro definitions confuse doxygen, causing it
    # to omit the respective elements in the documentation,
    # so ignore these macros during doxygen preprocessing:
    set(DOXYGEN_ENABLE_PREPROCESSING YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF NO)
    set(DOXYGEN_PREDEFINED
        "CESIUM3DTILES_API"
        "CESIUM3DTILESCONTENT_API"
        "CESIUM3DTILESREADER_API"
        "CESIUM3DTILESSELECTION_API"
        "CESIUM3DTILESWRITER_API"
        "CESIUMASYNC_API"
        "CESIUMGEOMETRY_API"
        "CESIUMGEOSPATIAL_API"
        "CESIUMGLTF_API"
        "CESIUMGLTFCONTENT_API"
        "CESIUMGLTFREADER_API"
        "CESIUMGLTFWRITER_API"
        "CESIUMIONCLIENT_API"
        "CESIUMJSONREADER_API"
        "CESIUMJSONWRITER_API"
        "CESIUMQUANTIZEDMESHTERRAIN_API"
        "CESIUMRASTEROVERLAYS_API"
        "CESIUMUTILITY_API"
        "CESIUM_DEFAULT_ELLIPSOID==CesiumGeospatial::Ellipsoid::WGS84")
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_CURRENT_LIST_DIR}/../node_modules/doxygen-awesome-css/doxygen-awesome.css")
    set(DOXYGEN_HTML_FOOTER "${CMAKE_CURRENT_LIST_DIR}/footer.html")
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "../README.md")

    set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)
    set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS_PRINT)
    list(APPEND DOXYGEN_IMAGE_PATH "${CMAKE_CURRENT_LIST_DIR}/")
    list(APPEND DOXYGEN_IMAGE_PATH "${CMAKE_CURRENT_LIST_DIR}/img")

    set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
    set(DOXYGEN_INTERACTIVE_SVG YES)
    # Tag files can be used by other Doxygen projects to link to our docs
    # For example, Cesium for Unreal can use it to link to the Cesium Native docs without duplicating them.
    set(DOXYGEN_GENERATE_TAGFILE "${CMAKE_CURRENT_BINARY_DIR}/html/cesium-native.tag")

    if(NOT CESIUM_PRODUCTION_DOCS)
      set(DOXYGEN_PROJECT_BRIEF "PRE-RELEASE")
      set(DOXYGEN_HTML_HEADER "${CMAKE_CURRENT_LIST_DIR}/header-noindex.html")
    else()
      set(DOXYGEN_HTML_HEADER "${CMAKE_CURRENT_LIST_DIR}/header.html")
    endif()

    # Add support for Mermaid charts using the @mermaid command.
    set(DOXYGEN_HTML_EXTRA_FILES "${CMAKE_CURRENT_LIST_DIR}/assets/mermaid.min.js ${CMAKE_CURRENT_LIST_DIR}/assets/mingcute.json.js ${CMAKE_CURRENT_LIST_DIR}/assets/panzoom.js")
    list(APPEND DOXYGEN_ALIASES mermaid{1}="\\htmlonly <div class=\\\"mermaid\\\"> \\ilinebr \\endhtmlonly \\htmlinclude \\\"\\1.mmd\\\" \\htmlonly \\ilinebr </div> \\endhtmlonly")
    list(APPEND DOXYGEN_ALIASES mermaid-interactive{1}="\\htmlonly <div class=\\\"mermaid interactive\\\"> \\ilinebr \\endhtmlonly \\htmlinclude \\\"\\1.mmd\\\" \\htmlonly \\ilinebr </div> \\endhtmlonly")
    list(APPEND DOXYGEN_ALIASES svg-interactive{2}="\\htmlonly <div class=\\\"svg interactive\\\" style=\\\"min-height: \\2\\\"> \\ilinebr \\endhtmlonly \\image html \\\"\\1.svg\\\" \\htmlonly \\ilinebr </div> \\endhtmlonly")
    set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES DOXYGEN_HTML_EXTRA_FILES)
    list(APPEND DOXYGEN_EXAMPLE_PATH "${CMAKE_CURRENT_LIST_DIR}/diagrams")

    list(APPEND DOXYGEN_TAGFILES "${CMAKE_CURRENT_LIST_DIR}/community.tag=https://github.com/CesiumGS/community/blob/main")

    cesium_glob_files(
      CESIUM_DOC_TOPICS
      ${CMAKE_CURRENT_LIST_DIR}/topics/*.md
    )

    doxygen_add_docs(
        cesium-native-docs
        "../CONTRIBUTING.md"
        "../README.md"
        "../CHANGES.md"
        ${CESIUM_DOC_TOPICS}
        ${LIB_DIRS}
    )
endif()
