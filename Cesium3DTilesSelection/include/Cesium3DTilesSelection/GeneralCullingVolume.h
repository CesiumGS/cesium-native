#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Library.h>
#include <CesiumGeometry/CullingVolume.h>

#include <variant>

namespace Cesium3DTilesSelection {

/**
 * @brief A generalization of @ref CesiumGeometry::CullingVolume.
 *
 * CesiumGeometry::CullingVolume represents orthographic and perspective view
 * frustums; GeneralCullingVolume includes @ref
 * Cesium3DTilesSelection::BoundingVolume too, allowing filtering on
 * e.g. geographic areas.
 */
using GeneralCullingVolume =
    std::variant<CesiumGeometry::CullingVolume, BoundingVolume>;

/**
 * @brief test if a general culling volume intersects a bounding volume.
 *
 * @param CullingVolume the general culling volume.
 * @param BoundingVolume the bounding volume.
 * @return true if volumes intersect.
 */
bool CESIUM3DTILESSELECTION_API isBoundingVolumeVisible(
    const GeneralCullingVolume& cullingVolume,
    const BoundingVolume& boundingVolume);

} // namespace Cesium3DTilesSelection
