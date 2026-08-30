#pragma once
#include <CesiumI3S/Library.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace CesiumI3S {

/** @brief Well-known spatial reference system (spatialReference.cmn.md). */
struct CESIUMI3S_API SpatialReference {
  /** @brief Most recent WKID for the vertical coordinate system. -1 = absent.
   */
  int32_t latestVcsWkid = -1;
  /** @brief Most recent WKID for the horizontal coordinate system. -1 = absent.
   */
  int32_t latestWkid = -1;
  /** @brief WKID for the vertical coordinate system. -1 = absent. */
  int32_t vcsWkid = -1;
  /** @brief WKID for the horizontal coordinate system. -1 = absent. */
  int32_t wkid = -1;
  /** @brief WKT representation of the coordinate system. Empty = absent. */
  std::string wkt;

  /** @brief Returns the effective horizontal WKID (prefers latestWkid). -1 if
   * neither is set. */
  int32_t horizontalWkid() const noexcept {
    return latestWkid != -1 ? latestWkid : wkid;
  }
  /** @brief Returns the effective vertical WKID (prefers latestVcsWkid). -1 if
   * neither is set. */
  int32_t verticalWkid() const noexcept {
    return latestVcsWkid != -1 ? latestVcsWkid : vcsWkid;
  }
};

/** @brief Height model for a layer (heightModelInfo.cmn.md). */
enum class HeightModel { GravityRelatedHeight, Ellipsoidal, Orthometric };

/** @brief Unit of measurement for heights (heightModelInfo.cmn.md). */
enum class HeightUnit {
  Meter,
  UsFoot,
  Foot,
  ClarkeFoot,
  ClarkeYard,
  ClarkeLink,
  SearsYard,
  SearsFoot,
  SearsChain,
  Benoit1895BChain,
  IndianYard,
  Indian1937Yard,
  GoldCoastFoot,
  Sears1922TruncatedChain,
  UsInch,
  UsMile,
  UsYard,
  Millimeter,
  Decimeter,
  Centimeter,
  Kilometer
};

/** @brief Coordinate system metadata for the layer's heights
 * (heightModelInfo.cmn.md). */
struct CESIUMI3S_API HeightModelInfo {
  /** @brief Height model. */
  std::optional<HeightModel> heightModel;
  /** @brief Vertical CRS name. */
  std::optional<std::string> vertCrs;
  /** @brief Unit of the height values. */
  std::optional<HeightUnit> heightUnit;
};

/** @brief Oriented bounding box (obb.cmn.md). */
struct CESIUMI3S_API OrientedBoundingBox {
  /** @brief Center position of the box. */
  std::array<double, 3> center{};
  /** @brief Half-extents of the box. */
  std::array<double, 3> halfSize{};
  /** @brief Rotation quaternion. */
  std::array<double, 4> quaternion{};
};

/** @brief Minimum Bounding Sphere. */
struct CESIUMI3S_API MinimumBoundingSphere {
  /** @brief Center position of the sphere. */
  std::array<double, 3> center{};
  /** @brief Radius of the sphere. */
  double radius = 0.0;
};

/** @brief Discriminates which bounding volume type is used for a node or
 * index. Both CMN and PCSL nodes use OrientedBoundingBox; legacy CMN nodes may
 * additionally carry a MinimumBoundingSphere. */
enum class BoundingVolume { OrientedBoundingBox, MinimumBoundingSphere };

/** @brief Elevation placement mode (elevationInfo.cmn.md,
 * elevationInfo.pcsl.md). */
enum class ElevationMode {
  /** @brief Height is relative to the ground surface. */
  RelativeToGround,
  /** @brief Height is an absolute elevation in meters. */
  AbsoluteHeight,
  /** @brief Feature is placed on the ground; height is ignored. */
  OnTheGround,
  /** @brief Height is relative to scene objects. */
  RelativeToScene
};

/** @brief Where features are placed vertically (elevationInfo.cmn.md). */
struct CESIUMI3S_API ElevationInfo {
  /** @brief Elevation placement mode. */
  std::optional<ElevationMode> mode;
  /** @brief Offset added to feature elevation in meters. */
  std::optional<double> offset;
  /** @brief Unit of the elevation values. */
  std::optional<HeightUnit> unit;
};

/** @brief 3D spatial extent of a layer (fullExtent.cmn.md). spatialReference,
 * if specified, must match the layer's spatialReference. */
struct CESIUMI3S_API FullExtent {
  /** @brief Minimum x-coordinate. */
  double xmin = 0.0;
  /** @brief Minimum y-coordinate. */
  double ymin = 0.0;
  /** @brief Maximum x-coordinate. */
  double xmax = 0.0;
  /** @brief Maximum y-coordinate. */
  double ymax = 0.0;
  /** @brief Minimum z-coordinate (elevation). */
  double zmin = 0.0;
  /** @brief Maximum z-coordinate (elevation). */
  double zmax = 0.0;
  /** @brief Spatial reference for extent coordinates. */
  std::optional<SpatialReference> spatialReference;
};

/** @brief 2D spatial extent [xmin, ymin, xmax, ymax] (store.cmn.md). */
struct CESIUMI3S_API Extent {
  /** @brief Minimum x-coordinate. */
  double xmin = 0.0;
  /** @brief Minimum y-coordinate. */
  double ymin = 0.0;
  /** @brief Maximum x-coordinate. */
  double xmax = 0.0;
  /** @brief Maximum y-coordinate. */
  double ymax = 0.0;
};

} // namespace CesiumI3S
