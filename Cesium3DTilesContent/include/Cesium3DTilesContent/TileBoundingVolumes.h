#pragma once

#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <optional>

namespace Cesium3DTiles {
struct BoundingVolume;
}

namespace Cesium3DTilesContent {

/**
 * @brief Provides functions for extracting bounding volumes types from the
 * vectors stored in {@link Cesium3DTiles::BoundingVolume}.
 */
class TileBoundingVolumes {
public:
  /**
   * @brief Gets the bounding box defined in a
   * {@link Cesium3DTiles::BoundingVolume}, if any.
   *
   * @param boundingVolume The bounding volume from which to get the box.
   * @return The box, or `std::nullopt` if the bounding volume does not
   * define a box. The box is defined in the tile's coordinate system.
   */
  static std::optional<CesiumGeometry::OrientedBoundingBox>
  getOrientedBoundingBox(const Cesium3DTiles::BoundingVolume& boundingVolume);

  /**
   * @brief Sets the `box` property in a {@link Cesium3DTiles::BoundingVolume}
   * based on an {@link CesiumGeometry::OrientedBoundingBox}.
   *
   * Other bounding volume types, if any, are not modified.
   *
   * @param boundingVolume The bounding volume to set.
   * @param boundingBox The bounding box with which to set the property.
   */
  static void setOrientedBoundingBox(
      Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeometry::OrientedBoundingBox& boundingBox);

  /**
   * @brief Gets the bounding region defined in a
   * {@link Cesium3DTiles::BoundingVolume}, if any.
   *
   * @param boundingVolume The bounding volume from which to get the region.
   * @param ellipsoid The ellipsoid on which the region should be defined.
   * @return The region, or `std::nullopt` if the bounding volume does not
   * define a region. The region is defined in geographic coordinates.
   */
  static std::optional<CesiumGeospatial::BoundingRegion> getBoundingRegion(
      const Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Sets the `region` property in a {@link Cesium3DTiles::BoundingVolume}
   * based on a {@link CesiumGeospatial::BoundingRegion}.
   *
   * Other bounding volume types, if any, are not modified.
   *
   * @param boundingVolume The bounding volume to set.
   * @param boundingRegion The bounding region with which to set the property.
   */
  static void setBoundingRegion(
      Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeospatial::BoundingRegion& boundingRegion);

  /**
   * @brief Gets the bounding sphere defined in a
   * {@link Cesium3DTiles::BoundingVolume}, if any.
   *
   * @param boundingVolume The bounding volume from which to get the sphere.
   * @return The sphere, or `std::nullopt` if the bounding volume does not
   * define a sphere. The sphere is defined in the tile's coordinate system.
   */
  static std::optional<CesiumGeometry::BoundingSphere>
  getBoundingSphere(const Cesium3DTiles::BoundingVolume& boundingVolume);

  /**
   * @brief Sets the `sphere` property in a {@link Cesium3DTiles::BoundingVolume}
   * based on a {@link CesiumGeometry::BoundingSphere}.
   *
   * Other bounding volume types, if any, are not modified.
   *
   * @param boundingVolume The bounding volume to set.
   * @param boundingSphere The bounding sphere with which to set the property.
   */
  static void setBoundingSphere(
      Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeometry::BoundingSphere& boundingSphere);

  /**
   * @brief Gets the S2 cell bounding volume defined in the
   * `3DTILES_bounding_volume_S2` extension of a
   * {@link Cesium3DTiles::BoundingVolume}, if any.
   *
   * @param boundingVolume The bounding volume from which to get the S2 cell
   * bounding volume.
   * @param ellipsoid The ellipsoid on which the S2 cell should be defined.
   * @return The S2 cell bounding volume, or `std::nullopt` if the bounding
   * volume does not define an S2 cell bounding volume. The S2 cell bounding
   * volume is defined in geographic coordinates.
   */
  static std::optional<CesiumGeospatial::S2CellBoundingVolume>
  getS2CellBoundingVolume(
      const Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Adds the `3DTILES_bounding_volume_S2` extension to a
   * {@link Cesium3DTiles::BoundingVolume} based on a
   * {@link CesiumGeospatial::S2CellBoundingVolume}.
   *
   * Other bounding volume types, if any, are not modified.
   *
   * @param boundingVolume The bounding volume to set.
   * @param s2BoundingVolume The S2 bounding volume with which to set the
   * property.
   */
  static void setS2CellBoundingVolume(
      Cesium3DTiles::BoundingVolume& boundingVolume,
      const CesiumGeospatial::S2CellBoundingVolume& s2BoundingVolume);
};

} // namespace Cesium3DTilesContent
