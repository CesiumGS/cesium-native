#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumVectorData/Library.h>

#include <glm/vec3.hpp>

#include <variant>
#include <vector>

namespace CesiumVectorData {

/**
 * @brief A type of object in GeoJson data.
 */
enum class GeoJsonObjectType : uint8_t {
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
 * @brief Returns the name of a `GeoJsonObjectType` value.
 */
std::string geoJsonObjectTypeToString(GeoJsonObjectType type);

/**
 * @brief A `Point` geometry object.
 *
 * A Point value is a single cartographic position.
 */
struct GeoJsonPoint {
  /** @brief The `GeoJsonObjectType` for a Point. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::Point;

  /**
   * @brief The `Cartographic` coordinates for this Point.
   */
  CesiumGeospatial::Cartographic coordinates;

  /**
   * @brief The bounding box associated with this Point value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `MultiPoint` geometry object.
 *
 * A MultiPoint value is a list of multiple cartographic positions.
 */
struct GeoJsonMultiPoint {
  /** @brief The `GeoJsonObjectType` for a MultiPoint. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::MultiPoint;

  /**
   * @brief The list of `Cartographic` coordinates for this MultiPoint.
   */
  std::vector<CesiumGeospatial::Cartographic> coordinates;

  /**
   * @brief The bounding box associated with this MultiPoint value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `LineString` geometry object.
 *
 * A LineString value is a list of two or more cartographic positions that form
 * a set of line segments.
 */
struct GeoJsonLineString {
  /** @brief The `GeoJsonObjectType` for a LineString. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::LineString;

  /**
   * @brief The list of `Cartographic` coordinates making up this LineString.
   */
  std::vector<CesiumGeospatial::Cartographic> coordinates;

  /**
   * @brief The bounding box associated with this LineString value, if any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `MultiLineString` geometry object.
 *
 * A MultiLineString value contains multiple lists of two or more points that
 * each make up a set of line segments.
 */
struct GeoJsonMultiLineString {
  /** @brief The `GeoJsonObjectType` for a MultiLineString. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::MultiLineString;

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
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
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
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::Polygon;

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
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A 'MultiPolygon' geometry object.
 *
 * A MultiPolygon value contains multiple Polygon coordinate sets.
 */
struct GeoJsonMultiPolygon {
  /** @brief The `GeoJsonObjectType` for a MultiPolygon. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::MultiPolygon;

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
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

struct GeoJsonGeometryCollection;

/**
 * @brief GeoJson objects that represent geometry.
 */
using GeoJsonGeometryObject = std::variant<
    GeoJsonPoint,
    GeoJsonMultiPoint,
    GeoJsonLineString,
    GeoJsonMultiLineString,
    GeoJsonPolygon,
    GeoJsonMultiPolygon,
    GeoJsonGeometryCollection>;

/**
 * @brief A `GeometryCollection` represents any number of \ref
 * GeoJsonGeometryObject objects.
 */
struct GeoJsonGeometryCollection {
  /** @brief The `GeoJsonObjectType` for a GeometryCollection. */
  static constexpr GeoJsonObjectType type =
      GeoJsonObjectType::GeometryCollection;

  /**
   * @brief The \ref GeometryObject values contained in this
   * GeometryCollection.
   */
  std::vector<GeoJsonGeometryObject> geometries;

  /**
   * @brief The bounding box associated with this GeometryCollection value, if
   * any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief A `Feature` object represents a spatially bounded "thing." It is a
 * collection of information that is possibly linked to a geometry object.
 */
struct GeoJsonFeature {
  /** @brief The `GeoJsonObjectType` for a Feature. */
  static constexpr GeoJsonObjectType type = GeoJsonObjectType::Feature;

  /**
   * @brief The "id" of this object. A Feature's ID is optional, but if
   * specified it will be either a string or a number.
   */
  std::variant<std::string, int64_t, std::monostate> id = std::monostate();

  /**
   * @brief The `GeoJsonGeometryObject` associated with this Feature, if any.
   */
  std::optional<GeoJsonGeometryObject> geometry;

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
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
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
struct GeoJsonFeatureCollection {
  /** @brief The `GeoJsonObjectType` for a FeatureCollection. */
  static constexpr GeoJsonObjectType type =
      GeoJsonObjectType::FeatureCollection;

  /**
   * @brief The \ref Feature objects contained in this FeatureCollection.
   */
  std::vector<GeoJsonFeature> features;

  /**
   * @brief The bounding box associated with this FeatureCollection value, if
   * any.
   */
  std::optional<CesiumGeospatial::BoundingRegion> boundingBox = std::nullopt;

  /**
   * @brief Any members specified on this object that are not part of the
   * specification for this object.
   *
   * See https://datatracker.ietf.org/doc/html/rfc7946#section-6.1 for more
   * information.
   */
  CesiumUtility::JsonValue::Object foreignMembers =
      CesiumUtility::JsonValue::Object();
};

/**
 * @brief Every possible object that can be specified in a GeoJSON
 * document.
 */
using GeoJsonObject = std::variant<
    GeoJsonPoint,
    GeoJsonMultiPoint,
    GeoJsonLineString,
    GeoJsonMultiLineString,
    GeoJsonPolygon,
    GeoJsonMultiPolygon,
    GeoJsonGeometryCollection,
    GeoJsonFeature,
    GeoJsonFeatureCollection>;

/**
 * @brief Converts a \ref GeoJsonGeometryObject to a \ref GeoJsonObject.
 *
 * All geometry objects are also valid objects, but not all objects are valid
 * geometry objects.
 */
GeoJsonObject
geoJsonGeometryObjectToObject(const GeoJsonGeometryObject& geometry);
} // namespace CesiumVectorData