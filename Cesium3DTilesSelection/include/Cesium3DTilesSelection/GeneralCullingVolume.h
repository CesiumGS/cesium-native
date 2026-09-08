#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Library.h>
#include <CesiumGeometry/CullingVolume.h>

#include <variant>

namespace Cesium3DTilesSelection {

/**
 * @brief A general culling volume for selecting and filtering tiles.
 *
 * While CesiumGeometry::CullingVolume represents orthographic and perspective
 * view frustums, GeneralCullingVolume also includes @ref
 * Cesium3DTilesSelection::BoundingVolume, which allows more general filtering
 * (e.g., on geographic areas).
 */
using GeneralCullingVolume =
    std::variant<CesiumGeometry::CullingVolume, BoundingVolume>;

/**
 * @brief Test if a general culling volume intersects a bounding volume.
 *
 * @param cullingVolume The general culling volume.
 * @param boundingVolume The bounding volume.
 * @return true if volumes intersect.
 */
bool CESIUM3DTILESSELECTION_API isBoundingVolumeVisible(
    const GeneralCullingVolume& cullingVolume,
    const BoundingVolume& boundingVolume);

} // namespace Cesium3DTilesSelection
