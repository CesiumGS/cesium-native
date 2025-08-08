#pragma once

#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorStyle.h>

#include <glm/vec3.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace CesiumVectorData {

/**
 * @brief A type of object in GeoJson data.
 */
enum class GeoJsonObjectType : uint8_t {
  Point = 0,
  MultiPoint = 1,
  LineString = 2,
  MultiLineString = 3,
  Polygon = 4,
  MultiPolygon = 5,
  GeometryCollection = 6,
  Feature = 7,
  FeatureCollection = 8
};

/**
 * @brief Returns the name of a `GeoJsonObjectType` value.
 */
std::string_view geoJsonObjectTypeToString(GeoJsonObjectType type);

/**
 * @brief A `Point` geometry object.
 *
 * A Point value is a single cartographic position.
 */
struct GeoJsonPoint {
  /** @brief The `GeoJsonObjectType` for a Point. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::Point;

  /**
   * @brief The `Cartographic` coordinates for this Point.
   */
  glm::dvec3 coordinates;

  /**
   * @brief The bounding box associated with this Point value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `MultiPoint` geometry object.
 *
 * A MultiPoint value is a list of multiple cartographic positions.
 */
struct GeoJsonMultiPoint {
  /** @brief The `GeoJsonObjectType` for a MultiPoint. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::MultiPoint;

  /**
   * @brief The list of `Cartographic` coordinates for this MultiPoint.
   */
  std::vector<glm::dvec3> coordinates;

  /**
   * @brief The bounding box associated with this MultiPoint value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `LineString` geometry object.
 *
 * A LineString value is a list of two or more cartographic positions that form
 * a set of line segments.
 */
struct GeoJsonLineString {
  /** @brief The `GeoJsonObjectType` for a LineString. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::LineString;

  /**
   * @brief The list of `Cartographic` coordinates making up this LineString.
   */
  std::vector<glm::dvec3> coordinates;

  /**
   * @brief The bounding box associated with this LineString value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `MultiLineString` geometry object.
 *
 * A MultiLineString value contains multiple lists of two or more points that
 * each make up a set of line segments.
 */
struct GeoJsonMultiLineString {
  /** @brief The `GeoJsonObjectType` for a MultiLineString. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::MultiLineString;

  /**
   * @brief The list of `Cartographic` coordinates making up this
   * MultiLineString.
   */
  std::vector<std::vector<glm::dvec3>> coordinates;

  /**
   * @brief The bounding box associated with this MultiLineString value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `Polygon` geometry object.
 *
 * A Polygon value contains multiple lists of four or more points that
 * each make up a "linear ring." Each linear ring is the boundary of the surface
 * or the boundary of a hole in that surface.
 */
struct GeoJsonPolygon {
  /** @brief The `GeoJsonObjectType` for a Polygon. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::Polygon;

  /**
   * @brief The list of linear rings making up this Polygon, each one defined by
   * a set of four or more `Cartographic` coordinates.
   *
   * Each linear ring can be thought of as a closed `LineString` - the first
   * and last positions must be equivalent and contain identical values. If more
   * than one of these rings is present, the first ring is the exterior
   * ring bounding the surface, and each additional ring represents the bounds
   * of holes within that surface.
   */
  std::vector<std::vector<glm::dvec3>> coordinates;

  /**
   * @brief The bounding box associated with this Polygon value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A 'MultiPolygon' geometry object.
 *
 * A MultiPolygon value contains multiple Polygon coordinate sets.
 */
struct GeoJsonMultiPolygon {
  /** @brief The `GeoJsonObjectType` for a MultiPolygon. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::MultiPolygon;

  /**
   * @brief The list of Polygons making up this MultiPolygon. Each entry has
   * equivalent rules to the \ref GeoJsonPolygon::coordinates "coordinates"
   * property of a \ref GeoJsonPolygon.
   */
  std::vector<std::vector<std::vector<glm::dvec3>>> coordinates;

  /**
   * @brief The bounding box associated with this MultiPolygon value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

struct GeoJsonObject;

/**
 * @brief A `GeometryCollection` represents any number of \ref
 * GeoJsonObject objects.
 */
struct GeoJsonGeometryCollection {
  /** @brief The `GeoJsonObjectType` for a GeometryCollection. */
  static constexpr GeoJsonObjectType TYPE =
      GeoJsonObjectType::GeometryCollection;

  /**
   * @brief The \ref GeoJsonObject values contained in this
   * GeometryCollection.
   */
  std::vector<GeoJsonObject> geometries;

  /**
   * @brief The bounding box associated with this GeometryCollection value, if
   * any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `GeoJsonFeature` object represents a spatially bounded "thing." It
 * is a collection of information that is possibly linked to a geometry object.
 */
struct GeoJsonFeature {
  /** @brief Default constructor. */
  GeoJsonFeature() = default;
  /** @brief Move constructor. */
  GeoJsonFeature(GeoJsonFeature&& rhs) noexcept = default;
  /** @brief Copy constructor. */
  GeoJsonFeature(const GeoJsonFeature& rhs);
  /**
   * @brief Creates a new \ref GeoJsonFeature with the given values.
   *
   * @param id The ID of the new feature.
   * @param geometry The GeoJSON geometry object contained in this \ref
   * GeoJsonFeature, if any.
   * @param properties Properties attached to this feature, if any.
   * @param boundingBox The bounding box defined for this feature, if any.
   * @param foreignMembers Any foreign members defined on this object in the
   * source GeoJSON.
   */
  GeoJsonFeature(
      std::variant<std::monostate, std::string, int64_t>&& id,
      std::unique_ptr<GeoJsonObject>&& geometry,
      std::optional<CesiumUtility::JsonValue::Object>&& properties,
      std::optional<CesiumGeometry::AxisAlignedBox>&& boundingBox,
      CesiumUtility::JsonValue::Object&& foreignMembers);
  /** @brief Copy assignment operator. */
  GeoJsonFeature& operator=(const GeoJsonFeature& rhs);
  /** @brief Move assignment operator. */
  GeoJsonFeature& operator=(GeoJsonFeature&& rhs) noexcept = default;

  /** @brief The `GeoJsonObjectType` for a Feature. */
  static constexpr GeoJsonObjectType TYPE = GeoJsonObjectType::Feature;

  /**
   * @brief The "id" of this object. A Feature's ID is optional, but if
   * specified it will be either a string or a number.
   */
  std::variant<std::monostate, std::string, int64_t> id = std::monostate();

  /**
   * @brief The `GeoJsonGeometryObject` associated with this Feature, if any.
   */
  std::unique_ptr<GeoJsonObject> geometry;

  /**
   * @brief The set of additional properties specified on this Feature, if any.
   *
   * The properties field may contain any valid JSON object.
   */
  std::optional<CesiumUtility::JsonValue::Object> properties;

  /**
   * @brief The bounding box associated with this Feature value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief A `FeatureCollection` represents any number of \ref GeoJsonFeature
 * objects.
 */
struct GeoJsonFeatureCollection {
  /** @brief The `GeoJsonObjectType` for a FeatureCollection. */
  static constexpr GeoJsonObjectType TYPE =
      GeoJsonObjectType::FeatureCollection;

  /**
   * @brief The \ref GeoJsonFeature objects contained in this FeatureCollection.
   */
  std::vector<GeoJsonObject> features;

  /**
   * @brief The bounding box associated with this GeoJsonFeatureCollection
   * value, if any.
   */
  std::optional<CesiumGeometry::AxisAlignedBox> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();

  /**
   * @brief The style to apply to this object as well as any child object. If
   * not set, the style of any parent object or the default style will be used.
   */
  std::optional<VectorStyle> style = std::nullopt;
};

/**
 * @brief Every possible object that can be specified in a GeoJSON
 * document.
 */
using GeoJsonObjectVariant = std::variant<
    GeoJsonPoint,
    GeoJsonMultiPoint,
    GeoJsonLineString,
    GeoJsonMultiLineString,
    GeoJsonPolygon,
    GeoJsonMultiPolygon,
    GeoJsonGeometryCollection,
    GeoJsonFeature,
    GeoJsonFeatureCollection>;
} // namespace CesiumVectorData