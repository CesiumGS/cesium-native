#pragma once

#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/GeoJsonObjectDescriptor.h"

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>

#include <glm/vec3.hpp>

#include <variant>
#include <vector>

namespace CesiumVectorData {

class GeoJsonDocument;

struct GeoJsonLineString {
  int32_t pointStartIndex;
  int32_t pointEndIndex;

  const std::span<CesiumGeospatial::Cartographic> getPoints(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;
};

struct GeoJsonPolygon {
  int32_t lineStringStartIndex;
  int32_t lineStringEndIndex;

  const std::span<GeoJsonLineString> getLineStrings(
      const CesiumUtility::IntrusivePointer<GeoJsonDocument>& pDocument) const;
};

/**
 * @brief A VectorFeature is a geometry object with attached metadata.
 *
 * A feature will be attached to one or more geometries.
 *
 * For example, take the following GeoJSON:
 * ```
 * {
 *   "type": "Point",
 *   "coordinates": [90.0, 45.0]
 * }
 * ```
 * This GeoJSON will be represented as a single VectorFeature containing one
 * Cartographic primitive.
 */
class CESIUMVECTORDATA_API GeoJsonFeature {
public:
  /**
   * @brief Constructs an empty VectorFeature.
   */
  GeoJsonFeature() = default;

  /**
   * @brief Constructs a VectorFeature containing a single primitive.
   *
   * @param geometry The \ref GeoJsonObjectDescriptor this node should contain.
   */
  GeoJsonFeature(GeoJsonObjectDescriptor&& geometry)
      : geometry(std::move(geometry)) {}

  /**
   * @brief Vector geometries contained by this node.
   *
   * For example, a GeoJSON `MultiPolygon` primitive contains one or more
   * `CesiumGeospatial::CartographicPolygon3D` primitives.
   */
  GeoJsonObjectDescriptor geometry;

  /**
   * @brief The "id" of this node. An ID, if specified, can be either a string
   * or a number.
   */
  std::variant<std::string, int64_t, std::monostate> id = std::monostate();
  /**
   * @brief The set of additional properties specified on this node, if any.
   *
   * The difference between `properties` and `foreignMembers` is that
   * `properties` contains additional data that is "supposed" to, as far as the
   * specification is concerned, appear on a node. For example, GeoJSON Feature
   * nodes have a "properties" member that allows for an arbitrary JSON object
   * to be specified on each Feature. In contrast, `foreignMembers` is meant to
   * contain properties that are not expected to be present on a node, but are
   * nevertheless there.
   */
  std::optional<CesiumUtility::JsonValue::Object> properties;

  /**
   * @brief Checks that two VectorNode values are equal.
   */
  bool operator==(const GeoJsonFeature& rhs) const;
};
} // namespace CesiumVectorData