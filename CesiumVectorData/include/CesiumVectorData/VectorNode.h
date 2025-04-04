#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>

#include <glm/vec3.hpp>

#include <variant>
#include <vector>

namespace CesiumVectorData {

using VectorPrimitive = std::variant<
    CesiumGeospatial::Cartographic,
    std::vector<CesiumGeospatial::Cartographic>,
    CesiumGeospatial::CompositeCartographicPolygon>;

/**
 * @brief A VectorNode is a single logical object in a VectorDocument's tree.
 *
 * A node will be attached to one or more primitives, and potentially some
 * number of child nodes.
 *
 * For example, take the following GeoJSON:
 * ```
 * {
 *   "type": "Point",
 *   "coordinates": [90.0, 45.0]
 * }
 * ```
 * This GeoJSON will be represented as a single VectorNode containing zero
 * children and one Cartographic primitive.
 */
class CESIUMVECTORDATA_API VectorNode {
public:
  VectorNode() = default;

  VectorNode(VectorPrimitive&& primitive) {
    this->primitives.emplace_back(std::move(primitive));
  }

  VectorNode(
      std::vector<VectorPrimitive>&& primitives,
      std::optional<CesiumGeospatial::BoundingRegion>&& boundingBox,
      CesiumUtility::JsonValue::Object&& foreignMembers =
          CesiumUtility::JsonValue::Object())
      : primitives(std::move(primitives)),
        boundingBox(std::move(boundingBox)),
        foreignMembers(std::move(foreignMembers)) {}

  VectorNode(std::vector<VectorNode>&& children)
      : children(std::move(children)) {}

  /**
   * @brief Nodes that are children of this node.
   *
   * For example, a GeoJSON `FeatureCollection` contains any number of features
   * that will each be treated as nodes that are children of their parent
   * `FeatureCollection` node.
   */
  std::vector<VectorNode> children;

  /**
   * @brief Vector primitives contained by this node.
   *
   * For example, a GeoJSON `MultiPolygon` primitive contains one or more
   * `CesiumGeospatial::CartographicPolygon3D` primitives.
   */
  std::vector<VectorPrimitive> primitives;

  /**
   * @brief The "id" of this node.
   *
   * The nodes that have IDs may vary between vector formats. In GeoJSON, only
   * Feature nodes can have IDs. An ID, if specified, can be either a string or
   * a number.
   */
  std::variant<std::string, int64_t, std::monostate> id = std::monostate();

  /**
   * @brief The bounding box associated with this node, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

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
   * @brief Any members specified on this node that are not part of the
   * specification for this node.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  bool operator==(const VectorNode& rhs) const;
};
} // namespace CesiumVectorData