#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>

#include <glm/vec3.hpp>

#include <variant>
#include <vector>

namespace CesiumVectorData {

/**
 * @brief A type of primitive in vector data.
 */
enum class VectorPrimitiveType : uint8_t {
  Point = 1,
  MultiPoint = 2,
  LineString = 3,
  MultiLineString = 4,
  Polygon = 5,
  MultiPolygon = 6,
  GeometryCollection = 7,
  Feature = 8,
  FeatureCollection = 9
};

/**
 * @brief Returns the name of a `VectorPrimitiveType` value.
 */
std::string vectorPrimitiveTypeToString(VectorPrimitiveType type);

/**
 * @brief A `Point` vector data primitive.
 *
 * A Point value is a single cartographic position.
 */
struct Point {
  /** @brief The `VectorPrimitiveType` for a Point. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Point;

  /**
   * @brief The `Cartographic` coordinates for this Point.
   */
  CesiumGeospatial::Cartographic coordinates;

  /**
   * @brief The bounding box associated with this Point value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `MultiPoint` vector data primitive.
 *
 * A MultiPoint value is a list of multiple cartographic positions.
 */
struct MultiPoint {
  /** @brief The `VectorPrimitiveType` for a MultiPoint. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::MultiPoint;

  /**
   * @brief The list of `Cartographic` coordinates for this MultiPoint.
   */
  std::vector<CesiumGeospatial::Cartographic> coordinates;

  /**
   * @brief The bounding box associated with this MultiPoint value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `LineString` vector data primitive.
 *
 * A LineString value is a list of two or more cartographic positions that form
 * a set of line segments.
 */
struct LineString {
  /** @brief The `VectorPrimitiveType` for a LineString. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::LineString;

  /**
   * @brief The list of `Cartographic` coordinates making up this LineString.
   */
  std::vector<CesiumGeospatial::Cartographic> coordinates;

  /**
   * @brief The bounding box associated with this LineString value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `MultiLineString` vector data primitive.
 *
 * A MultiLineString value contains multiple lists of two or more points that
 * each make up a set of line segments.
 */
struct MultiLineString {
  /** @brief The `VectorPrimitiveType` for a MultiLineString. */
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::MultiLineString;

  /**
   * @brief The list of `Cartographic` coordinates making up this
   * MultiLineString.
   */
  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;

  /**
   * @brief The bounding box associated with this MultiLineString value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `Polygon` vector data primitive.
 *
 * A Polygon value contains multiple lists of four or more points that
 * each make up a "linear ring." Each linear ring is the boundary of the surface
 * or the boundary of a hole in that surface.
 */
struct Polygon {
  /** @brief The `VectorPrimitiveType` for a Polygon. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  /**
   * @brief The list of linear rings making up this Polygon, each one defined by
   * a set of four or more `Cartographic` coordinates.
   *
   * Each linear ring can be thought of as a as closed `LineString` - the first
   * and last positions must be equivalent and contain identical values. If more
   * than one of these rings is present, the first ring represents the exterior
   * ring bounding the surface, and each additional ring represents the bounds
   * of holes within that surface.
   */
  std::vector<std::vector<CesiumGeospatial::Cartographic>> coordinates;

  /**
   * @brief The bounding box associated with this Polygon value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A 'MultiPolygon' vector data primitive.
 *
 * A MultiPolygon value contains multiple Polygon coordinate sets.
 */
struct MultiPolygon {
  /** @brief The `VectorPrimitiveType` for a MultiPolygon. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Polygon;

  /**
   * @brief The list of Polygons making up this MultiPolygon. Each entry has
   * equivalent rules to the \ref Polygon::coordinates "coordinates" property of
   * a \ref Polygon.
   */
  std::vector<std::vector<std::vector<CesiumGeospatial::Cartographic>>>
      coordinates;

  /**
   * @brief The bounding box associated with this MultiPolygon value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct GeometryCollection;

/**
 * @brief Vector primitives considered to represent geometry.
 */
using GeometryPrimitive = std::variant<
    Point,
    MultiPoint,
    LineString,
    MultiLineString,
    Polygon,
    MultiPolygon,
    GeometryCollection>;

/**
 * @brief A `GeometryCollection` represents any number of \ref GeometryPrimitive
 * objects.
 */
struct GeometryCollection {
  /** @brief The `VectorPrimitiveType` for a GeometryCollection. */
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::GeometryCollection;

  /**
   * @brief The \ref GeometryPrimitive values contained in this
   * GeometryCollection.
   */
  std::vector<GeometryPrimitive> geometries;

  /**
   * @brief The bounding box associated with this GeometryCollection value, if
   * any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `Feature` object represents a spatially bounded "thing." It is a
 * collection of information that is possibly linked to a geometry primitive.
 */
struct Feature {
  /** @brief The `VectorPrimitiveType` for a Feature. */
  static constexpr VectorPrimitiveType Type = VectorPrimitiveType::Feature;

  /**
   * @brief The "id" of this primitive. A Feature's ID is optional, but if
   * specified it will be either a string or a number.
   */
  std::variant<std::string, int64_t, std::monostate> id = std::monostate();

  /**
   * @brief The `GeometryPrimitive` associated with this Feature, if any.
   */
  std::optional<GeometryPrimitive> geometry;

  /**
   * @brief The set of additional properties specified on this Feature, if any.
   *
   * The properties field may contain any valid JSON object.
   */
  std::optional<CesiumUtility::JsonValue::Object> properties;

  /**
   * @brief The bounding box associated with this Feature value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `FeatureCollection` represents any number of \ref Feature objects.
 */
struct FeatureCollection {
  /** @brief The `VectorPrimitiveType` for a FeatureCollection. */
  static constexpr VectorPrimitiveType Type =
      VectorPrimitiveType::FeatureCollection;

  /**
   * @brief The \ref Feature objects contained in this FeatureCollection.
   */
  std::vector<Feature> features;

  /**
   * @brief The bounding box associated with this FeatureCollection value, if
   * any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this primitive that are not part of the
   * specification for this primitive.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief Every possible vector primitive that can be specified in a vector
 * document.
 */
using VectorNode = std::variant<
    Point,
    MultiPoint,
    LineString,
    MultiLineString,
    Polygon,
    MultiPolygon,
    GeometryCollection,
    Feature,
    FeatureCollection>;

/**
 * @brief Converts a \ref GeometryPrimitive to a \ref VectorNode.
 *
 * All geometry primitives are also valid vector primitives, but not all vector
 * primitives are valid geometry primitives.
 */
VectorNode geometryPrimitiveToVectorNode(const GeometryPrimitive& geometry);

/** @brief Checks if two \ref VectorNode values are equal. */
bool operator==(const VectorNode& lhs, const VectorNode& rhs);
/** @brief Checks if two \ref GeometryPrimitive values are equal. */
bool operator==(const GeometryPrimitive& lhs, const GeometryPrimitive& rhs);
/** @brief Checks if a \ref Point is equal to another vector primitive. */
bool operator==(const Point& lhs, const VectorNode& rhs);
/** @brief Checks if a \ref MultiPoint is equal to another vector primitive. */
bool operator==(const MultiPoint& lhs, const VectorNode& rhs);
/** @brief Checks if a \ref LineString is equal to another vector primitive. */
bool operator==(const LineString& lhs, const VectorNode& rhs);
/**
 * @brief Checks if a \ref MultiLineString is equal to another vector
 * primitive.
 */
bool operator==(const MultiLineString& lhs, const VectorNode& rhs);
/** @brief Checks if a \ref Polygon is equal to another vector primitive. */
bool operator==(const Polygon& lhs, const VectorNode& rhs);
/**
 * @brief Checks if a \ref MultiPolygon is equal to another vector primitive.
 */
bool operator==(const MultiPolygon& lhs, const VectorNode& rhs);
/**
 * @brief Checks if a \ref GeometryCollection is equal to another vector
 * primitive.
 */
bool operator==(const GeometryCollection& lhs, const VectorNode& rhs);
/** @brief Checks if two \ref Feature values are equal. */
bool operator==(const Feature& lhs, const Feature& rhs);
/** @brief Checks if a \ref Feature is equal to another vector primitive. */
bool operator==(const Feature& lhs, const VectorNode& rhs);
/**
 * @brief Checks if a \ref FeatureCollection is equal to another vector
 * primitive.
 */
bool operator==(const FeatureCollection& lhs, const VectorNode& rhs);
} // namespace CesiumVectorData