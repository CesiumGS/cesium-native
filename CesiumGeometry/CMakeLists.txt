add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

set(CESIUM_GEOMETRY_SOURCE
    src/BoundingSphere.cpp
    src/clipTriangleAtAxisAlignedThreshold.cpp
    src/CullingVolume.cpp
    src/IntersectionTests.cpp
    src/OrientedBoundingBox.cpp
    src/Plane.cpp
    src/QuadtreeTileAvailability.cpp
    src/QuadtreeTileID.cpp
    src/QuadtreeTilingScheme.cpp
    src/Ray.cpp
    src/Rectangle.cpp
)

set(CESIUM_GEOMETRY_TEST_SOURCE
    test/TestBoundingSphere.cpp
    test/TestIntersectionTests.cpp
    test/TestOrientedBoundingBox.cpp
    test/TestPlane.cpp
    test/TestRectangle.cpp
)

list(TRANSFORM CESIUM_GEOMETRY_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_GEOMETRY_TEST_SOURCE "${CESIUM_GEOMETRY_TEST_SOURCE}" PARENT_SCOPE)

set(CESIUM_GEOMETRY_PUBLIC_HEADERS
    include/CesiumGeometry/BoundingSphere.h
    include/CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h
    include/CesiumGeometry/CullingResult.h
    include/CesiumGeometry/CullingVolume.h
    include/CesiumGeometry/IntersectionTests.h
    include/CesiumGeometry/Library.h
    include/CesiumGeometry/OctreeTileID.h
    include/CesiumGeometry/OrientedBoundingBox.h
    include/CesiumGeometry/Plane.h
    include/CesiumGeometry/QuadtreeTileAvailability.h
    include/CesiumGeometry/QuadtreeTileID.h
    include/CesiumGeometry/QuadtreeTileRectangularRange.h
    include/CesiumGeometry/QuadtreeTilingScheme.h
    include/CesiumGeometry/Ray.h
    include/CesiumGeometry/Rectangle.h
)

set(CESIUM_GEOMETRY_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include" PARENT_SCOPE)

target_sources(
    CesiumGeometry
    PRIVATE
        ${CESIUM_GEOMETRY_SOURCE}
    PUBLIC
        ${CESIUM_GEOMETRY_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumGeometry
    SYSTEM PUBLIC
        ${CESIUM_UTILITY_PUBLIC_HEADERS}
    PUBLIC
        include
)

target_link_libraries(CesiumGeometry PUBLIC CesiumUtility)
