#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Library.h>

#include <glm/mat4x4.hpp>

#include <optional>

namespace CesiumGeospatial {

class LocalHorizontalCoordinateSystem;

/**
 * @brief Anchors an object to the globe by defining a transformation from the
 * object's coordinate to the globe-fixed coordinate system (usually
 * \ref what-are-ecef-coordinates).
 *
 * This class allows the anchored coordinate system to be realized in any
 * {@link LocalHorizontalCoordinateSystem}. When the object is moved, either by
 * specifying a new globe-fixed transform or a new local transform, the
 * orientation may optionally be updated to keep the object upright at its new
 * location on the globe.
 */
class CESIUMGEOSPATIAL_API GlobeAnchor final {
public:
  /**
   * @brief Creates a new instance from a transformation to a local coordinate
   * system.
   *
   * For example, in a game engine, the transformation may be the game
   * object's initial transformation to the game engine's world coordinate
   * system.
   *
   * @param localCoordinateSystem The local coordinate system that is the target
   * of the transformation.
   * @param anchorToLocal The matrix transforming from this object's
   * coordinate system to the local coordinate system.
   */
  static GlobeAnchor fromAnchorToLocalTransform(
      const LocalHorizontalCoordinateSystem& localCoordinateSystem,
      const glm::dmat4& anchorToLocal);

  /**
   * @brief Creates a new instance from a transformation to the globe-fixed
   * coordinate system.
   *
   * @param anchorToFixed The matrix transforming from this object's
   * coordinate system to the globe-fixed coordinate system.
   */
  static GlobeAnchor
  fromAnchorToFixedTransform(const glm::dmat4& anchorToFixed);

  /**
   * @brief Constructs a new instance with a given transformation to the
   * globe-fixed coordinate system.
   *
   * @param anchorToFixed The matrix transforming from this object's
   * coordinate system to the globe-fixed coordinate system.
   */
  explicit GlobeAnchor(const glm::dmat4& anchorToFixed);

  /**
   * @brief Gets the transformation from the anchor's coordinate system to the
   * globe-fixed coordinate system.
   */
  const glm::dmat4& getAnchorToFixedTransform() const;

  /**
   * @brief Sets the new transformation from the anchor's coordinate system
   * to globe-fixed coordinates.
   *
   * @param newAnchorToFixed The new matrix transforming from this object's
   * coordinate system to the globe-fixed coordinate system.
   * @param adjustOrientation Whether to adjust the anchor's orientation based
   * on globe curvature as the anchor moves. The Earth is not flat, so as we
   * move across its surface, the direction of "up" changes. If we ignore this
   * fact and leave an object's orientation unchanged as it moves over the globe
   * surface, the object will become increasingly tilted and eventually be
   * completely upside-down when we arrive at the opposite side of the globe.
   * When this parameter is true, this method will automatically apply a
   * rotation to the anchor to account for globe curvature when the position on
   * the globe changes. This parameter should usually be true, but it may be
   * useful to set it to false it when the caller already accounts for globe
   * curvature itself, because in that case anchor would be over-rotated.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   */
  void setAnchorToFixedTransform(
      const glm::dmat4& newAnchorToFixed,
      bool adjustOrientation,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Gets the transformation from the anchor's coordinate system to the
   * given local-horizontal coordinate system.
   */
  glm::dmat4 getAnchorToLocalTransform(
      const LocalHorizontalCoordinateSystem& localCoordinateSystem) const;

  /**
   * @brief Sets the globe-fixed transformation based on a new transformation
   * from anchor coordinates to a local-horizontal coordinate system.
   *
   * For example, in a game engine, the new transformation may be the game
   * object's normal transformation to the game engine's world coordinate
   * system. It may need to be updated because the object was just moved by the
   * physics engine.
   *
   * @param localCoordinateSystem The local coordinate system that is the target
   * of the transformation.
   * @param newAnchorToLocal The new matrix transforming from this object's
   * coordinate system to the local coordinate system.
   * @param adjustOrientation Whether to adjust the anchor's orientation based
   * on globe curvature as the anchor moves. The Earth is not flat, so as we
   * move across its surface, the direction of "up" changes. If we ignore this
   * fact and leave an object's orientation unchanged as it moves over the globe
   * surface, the object will become increasingly tilted and eventually be
   * completely upside-down when we arrive at the opposite side of the globe.
   * When this parameter is true, this method will automatically apply a
   * rotation to the anchor to account for globe curvature when the position on
   * the globe changes. This parameter should usually be true, but it may be
   * useful to set it to false it when the caller already accounts for globe
   * curvature itself, because in that case anchor would be over-rotated.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   */
  void setAnchorToLocalTransform(
      const LocalHorizontalCoordinateSystem& localCoordinateSystem,
      const glm::dmat4& newAnchorToLocal,
      bool adjustOrientation,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

private:
  glm::dmat4 _anchorToFixed;
};

} // namespace CesiumGeospatial
