#pragma once

#include <Cesium3DTiles/Library.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumUtility/Result.h>

#include <optional>
#include <variant>

namespace CesiumGltf {
struct BoundingVolume;
struct Model;
} // namespace CesiumGltf

namespace CesiumTilesetJson {
struct BoundingVolume;
} // namespace CesiumTilesetJson

namespace Cesium3DTiles {
/**
 * @brief A bounding volume.
 *
 * This is a `std::variant` for different types of bounding volumes.
 *
 * @see CesiumGeometry::BoundingSphere
 * @see CesiumGeometry::OrientedBoundingBox
 * @see CesiumGeospatial::BoundingRegion
 * @see CesiumGeospatial::BoundingRegionWithLooseFittingHeights
 * @see CesiumGeospatial::S2CellBoundingVolume
 * @see CesiumGeometry::BoundingCylinderRegion
 */
typedef std::variant<
    CesiumGeometry::BoundingSphere,
    CesiumGeometry::OrientedBoundingBox,
    CesiumGeospatial::BoundingRegion,
    CesiumGeospatial::BoundingRegionWithLooseFittingHeights,
    CesiumGeospatial::S2CellBoundingVolume,
    CesiumGeometry::BoundingCylinderRegion>
    BoundingVolume;

/**
 * @brief Creates a @ref Cesium3DTiles::BoundingVolume from a @ref
 * CesiumTilesetJson::BoundingVolume, if possible.
 *
 * If the resulting volume is a @ref CesiumGeospatial::BoundingRegion or @ref
 * CesiumGeospatial::S2CellBoundingVolume, then the volume is defined in
 * geographic coordinates. Otherwise, the volume is defined relative to the
 * coordinate system of the tile it came from.
 *
 * @param boundingVolume The bounding volume from the tileset.json.
 * @param ellipsoid The ellipsoid to use for globe calculations.
 * @return The bounding volume, or a list of errors if construction was not
 * successful.
 */
CESIUM3DTILES_API CesiumUtility::Result<BoundingVolume> getBoundingVolume(
    const CesiumTilesetJson::BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Creates a @ref Cesium3DTiles::BoundingVolume from a @ref
 * CesiumGltf::BoundingVolume in a @ref CesiumGltf::Model, if possible.
 *
 * If the resulting volume is a @ref CesiumGeospatial::BoundingRegion or @ref
 * CesiumGeospatial::S2CellBoundingVolume, then the volume is defined in
 * geographic coordinates. Otherwise, the volume is defined relative to the
 * coordinate system of the node it came from.
 *
 * @param model The glTF model.
 * @param boundingVolume The bounding volume from the glTF model.
 * @param ellipsoid The ellipsoid to use for globe calculations.
 * @return The bounding volume, or a list of errors if construction was not
 * successful.
 */
CESIUM3DTILES_API CesiumUtility::Result<BoundingVolume> getBoundingVolume(
    const CesiumGltf::Model& model,
    const CesiumGltf::BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Transform the given @ref BoundingVolume with the given matrix.
 *
 * If the given bounding volume is a @ref CesiumGeometry::BoundingSphere
 * or @ref CesiumGeometry::OrientedBoundingBox, then it will be transformed
 * with the given matrix. Bounding regions will not be transformed.
 *
 * @param transform The transform matrix.
 * @param boundingVolume The bounding volume.
 * @return The transformed bounding volume.
 */
CESIUM3DTILES_API BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume);

/**
 * @brief Returns the center of the given @ref BoundingVolume.
 *
 * @param boundingVolume The bounding volume.
 * @return The center point.
 */
CESIUM3DTILES_API glm::dvec3
getBoundingVolumeCenter(const BoundingVolume& boundingVolume);

/**
 * @brief Estimates the bounding @ref CesiumGeospatial::GlobeRectangle of the
 * given @ref BoundingVolume.
 *
 * @param boundingVolume The bounding volume.
 * @param ellipsoid The ellipsoid to use for globe calculations.
 * @return The bounding @ref CesiumGeospatial::GlobeRectangle.
 */
CESIUM3DTILES_API std::optional<CesiumGeospatial::GlobeRectangle>
estimateGlobeRectangle(
    const BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Returns the bounding region if the bounding volume is a
 * @ref CesiumGeospatial::BoundingRegion or a @ref
 * CesiumGeospatial::BoundingRegionWithLooseFittingHeights.
 *
 * @param boundingVolume The bounding volume.
 * @return A pointer to the bounding region, or nullptr is the bounding volume
 * is not a bounding region.
 */
CESIUM3DTILES_API const CesiumGeospatial::BoundingRegion*
getBoundingRegionFromBoundingVolume(const BoundingVolume& boundingVolume);

/**
 * @brief Returns an oriented bounding box that contains the given @ref
 * BoundingVolume.
 *
 * @param boundingVolume The bounding volume.
 * @param ellipsoid The ellipsoid used for this @ref BoundingVolume.
 * @return The oriented bounding box.
 */
CESIUM3DTILES_API CesiumGeometry::OrientedBoundingBox
getOrientedBoundingBoxFromBoundingVolume(
    const BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

/**
 * @brief Returns true if two bounding volumes intersect.
 */
CESIUM3DTILES_API bool
testIntersection(const BoundingVolume& volume0, const BoundingVolume& volume1);
} // namespace Cesium3DTiles
