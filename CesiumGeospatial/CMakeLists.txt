add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

set(CESIUM_GEOSPATIAL_SOURCE
    src/BoundingRegion.cpp
    src/BoundingRegionWithLooseFittingHeights.cpp
    src/Ellipsoid.cpp
    src/EllipsoidTangentPlane.cpp
    src/GeographicProjection.cpp
    src/GlobeRectangle.cpp
    src/Projection.cpp
    src/Transforms.cpp
    src/WebMercatorProjection.cpp
)

set(CESIUM_GEOSPATIAL_PUBLIC_HEADERS
    include/CesiumGeospatial/BoundingRegion.h
    include/CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h
    include/CesiumGeospatial/Cartographic.h
    include/CesiumGeospatial/Ellipsoid.h
    include/CesiumGeospatial/EllipsoidTangentPlane.h
    include/CesiumGeospatial/GeographicProjection.h
    include/CesiumGeospatial/GlobeRectangle.h
    include/CesiumGeospatial/Library.h
    include/CesiumGeospatial/Projection.h
    include/CesiumGeospatial/Transforms.h
    include/CesiumGeospatial/WebMercatorProjection.h
)

set(CESIUM_GEOSPATIAL_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/" PARENT_SCOPE)

set(CESIUM_GEOSPATIAL_TEST_SOURCE
    test/TestBoundingRegion.cpp
    test/TestGlobeRectangle.cpp
)

list(TRANSFORM CESIUM_GEOSPATIAL_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_GEOSPATIAL_TEST_SOURCE "${CESIUM_GEOSPATIAL_TEST_SOURCE}" PARENT_SCOPE)

target_sources(
    CesiumGeospatial
    PRIVATE
        ${CESIUM_GEOSPATIAL_SOURCE}
    PUBLIC
        ${CESIUM_GEOSPATIAL_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeospatial
    PUBLIC
        include
)

target_link_libraries(CesiumGeospatial PUBLIC CesiumGeometry CesiumUtility)
