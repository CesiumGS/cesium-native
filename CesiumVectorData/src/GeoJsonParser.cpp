#include "GeoJsonParser.h"

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorNode.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstddef>
#include <functional>
#include <span>
#include <variant>

using namespace CesiumUtility;
using namespace CesiumGeospatial;

namespace CesiumVectorData {

namespace {
/**
 * @brief Collects foreign members (other JSON members outside the
 * specification) into a `JsonValue::Object`
 *
 * @param obj The JSON object to collect foreign members from.
 * @param predicate Receives the name of the member and returns whether the
 * member is known. Returning false adds the object to the foreign member set.
 */
JsonValue::Object collectForeignMembers(
    const rapidjson::Value::Object& obj,
    const std::function<bool(const std::string& key)>& predicate) {
  JsonValue::Object newObj;

  for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
    const std::string& name = it->name.GetString();
    if (name != "type" && name != "bbox" && !predicate(name)) {
      newObj.emplace(name, JsonHelpers::toJsonValue(it->value));
    }
  }

  return newObj;
}

// GeoJSON position is an array of two or three components, representing
// [longitude, latitude, (height)]
Result<Cartographic> parsePosition(const rapidjson::Value& pos) {
  if (!pos.IsArray() || pos.GetArray().Size() < 2 ||
      pos.GetArray().Size() > 3) {
    return Result<Cartographic>(
        ErrorList::error("Position value must be an array."));
  }

  const int32_t size = static_cast<int32_t>(pos.GetArray().Size());
  if (!pos.IsArray() || size < 2 || size > 3) {
    return Result<Cartographic>(ErrorList::error(
        "Position value must be an array with two or three members."));
  }

  std::optional<std::vector<double>> components =
      JsonHelpers::getDoubles(pos, size);

  if (!components) {
    return Result<Cartographic>(
        ErrorList::error("Position value must be an array of only numbers."));
  }

  return Cartographic(
      Math::degreesToRadians((*components)[0]),
      Math::degreesToRadians((*components)[1]),
      size == 3 ? (*components)[2] : 0);
}

Result<std::vector<Cartographic>>
parsePositionArray(const rapidjson::Value::Array& arr) {
  std::vector<Cartographic> points;
  points.reserve(arr.Size());
  for (auto& value : arr) {
    Result<Cartographic> coordinateResult = parsePosition(value);
    if (!coordinateResult.value) {
      return Result<std::vector<Cartographic>>(
          std::move(coordinateResult.errors));
    }
    points.emplace_back(std::move(*coordinateResult.value));
  }

  return Result<std::vector<Cartographic>>(std::move(points));
}

Result<std::vector<std::vector<Cartographic>>> parsePolygon(
    const rapidjson::Value::Array& arr,
    const std::string& name,
    uint32_t minItems) {
  std::vector<std::vector<Cartographic>> rings;
  rings.reserve(arr.Size());
  for (auto& value : arr) {
    if (!value.IsArray()) {
      return Result<std::vector<std::vector<Cartographic>>>(ErrorList::error(
          name + " 'coordinates' member must be an array of position arrays."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(value.GetArray());
    if (!pointsResult.value) {
      return Result<std::vector<std::vector<Cartographic>>>(
          std::move(pointsResult.errors));
    } else if (pointsResult.value->size() < minItems) {
      return Result<std::vector<std::vector<Cartographic>>>(
          ErrorList::error(fmt::format(
              "{} 'coordinates' member must be an array of "
              "arrays of {} or more positions.",
              name,
              minItems)));
    }

    rings.emplace_back(std::move(*pointsResult.value));
  }

  return Result<std::vector<std::vector<Cartographic>>>(std::move(rings));
}

Result<std::optional<BoundingRegion>>
parseBoundingBox(const rapidjson::Value& value) {
  if (!value.IsArray()) {
    return Result<std::optional<BoundingRegion>>(
        std::nullopt,
        ErrorList::warning("'bbox' member must be an array."));
  }

  const int32_t size = static_cast<int32_t>(value.Size());
  if (size != 4 && size != 6) {
    return Result<std::optional<BoundingRegion>>(
        std::nullopt,
        ErrorList::warning(
            "'bbox' member must be of length 4 (2D) or 6 (3D)."));
  }

  const std::optional<std::vector<double>> doubles =
      JsonHelpers::getDoubles(value, size);
  if (!doubles) {
    return Result<std::optional<BoundingRegion>>(
        std::nullopt,
        ErrorList::warning("'bbox' member contain only doubles."));
  }

  if (size == 4) {
    return Result<std::optional<BoundingRegion>>(BoundingRegion(
        GlobeRectangle(
            Math::degreesToRadians((*doubles)[0]),
            Math::degreesToRadians((*doubles)[1]),
            Math::degreesToRadians((*doubles)[2]),
            Math::degreesToRadians((*doubles)[3])),
        0,
        0));
  }

  return Result<std::optional<BoundingRegion>>(BoundingRegion(
      GlobeRectangle(
          Math::degreesToRadians((*doubles)[0]),
          Math::degreesToRadians((*doubles)[1]),
          Math::degreesToRadians((*doubles)[3]),
          Math::degreesToRadians((*doubles)[4])),
      (*doubles)[2],
      (*doubles)[5]));
}

struct NodeToGeometryPrimitiveVisitor {
  Result<GeometryPrimitive> operator()(Point&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(MultiPoint&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(LineString&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(MultiLineString&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(Polygon&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(MultiPolygon&& node) {
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(GeometryCollection&& node) {
    // The specification says that implementations should avoid nesting a
    // GeometryCollection inside a GeometryCollection. However, I don't think it
    // costs us anything to handle it even if the data is ill-advised.
    return Result<GeometryPrimitive>(std::move(node));
  }
  Result<GeometryPrimitive> operator()(Feature&& /*node*/) {
    return Result<GeometryPrimitive>(
        ErrorList::error("Expected geometry, found GeoJSON object Feature."));
  }
  Result<GeometryPrimitive> operator()(FeatureCollection&& /*node*/) {
    return Result<GeometryPrimitive>(ErrorList::error(
        "Expected geometry, found GeoJSON object FeatureCollection."));
  }
};

struct NodeToFeatureVisitor {
  Result<Feature> operator()(Feature&& node) {
    return Result<Feature>(std::move(node));
  }

  Result<Feature> operator()(auto&& node) {
    return Result<Feature>(ErrorList::error(fmt::format(
        "Expected Feature, found GeoJSON object {}.",
        vectorPrimitiveTypeToString(node.Type))));
  }
};

Result<VectorNode> parseGeoJsonObject(const rapidjson::Value::Object& obj) {
  const std::string& type = JsonHelpers::getStringOrDefault(obj, "type", "");
  if (type.empty()) {
    return Result<VectorNode>(
        ErrorList::error("GeoJSON object missing required 'type' field."));
  }

  ErrorList errorList;
  // Try reading the bounding box
  std::optional<BoundingRegion> boundingBox;
  const auto bboxMember = obj.FindMember("bbox");
  if (bboxMember != obj.MemberEnd()) {
    Result<std::optional<BoundingRegion>> regionResult =
        parseBoundingBox(bboxMember->value);
    errorList.merge(regionResult.errors);
    boundingBox = std::move(*regionResult.value);
  }

  if (type == "Feature") {
    // Feature has a geometry, properties, and an optional id
    std::variant<std::string, int64_t, std::monostate> id;
    const auto& idMember = obj.FindMember("id");
    if (idMember != obj.MemberEnd()) {
      if (idMember->value.IsNumber()) {
        id = idMember->value.GetInt64();
      } else if (idMember->value.IsString()) {
        id = idMember->value.GetString();
      } else {
        return Result<VectorNode>(ErrorList::error(
            "Feature 'id' member must be either a string or a number."));
      }
    }

    const auto& geometryMember = obj.FindMember("geometry");
    if (geometryMember == obj.MemberEnd()) {
      return Result<VectorNode>(
          ErrorList::error("Feature must have 'geometry' member."));
    }

    std::optional<GeometryPrimitive> geometry = std::nullopt;
    if (!geometryMember->value.IsNull()) {
      if (!geometryMember->value.IsObject()) {
        return Result<VectorNode>(ErrorList::error(
            "Feature 'geometry' member must be either an object or null."));
      }

      Result<VectorNode> childResult =
          parseGeoJsonObject(geometryMember->value.GetObject());
      if (!childResult.value) {
        return childResult;
      }

      Result<GeometryPrimitive> geometryResult = std::visit(
          NodeToGeometryPrimitiveVisitor{},
          std::move(*childResult.value));
      if (!geometryResult.value) {
        return Result<VectorNode>(std::move(geometryResult.errors));
      }

      geometry = std::move(*geometryResult.value);
    }

    const auto& propertiesMember = obj.FindMember("properties");
    if (propertiesMember == obj.MemberEnd()) {
      return Result<VectorNode>(
          ErrorList::error("Feature must have 'properties' member."));
    }

    std::optional<JsonValue::Object> properties = std::nullopt;
    if (!propertiesMember->value.IsNull()) {
      if (!propertiesMember->value.IsObject()) {
        return Result<VectorNode>(ErrorList::error(
            "Feature 'properties' member must be either an object or null."));
      }

      properties = JsonHelpers::toJsonValue(propertiesMember->value.GetObject())
                       .getObject();
    }

    return Result<VectorNode>(Feature{
        id,
        geometry,
        properties,
        boundingBox,
        collectForeignMembers(obj, [](const std::string& k) {
          return k == "id" || k == "geometry" || k == "properties";
        })});
  } else if (type == "FeatureCollection") {
    // Feature collection contains zero or more features
    const auto& featuresMember = obj.FindMember("features");
    if (featuresMember == obj.MemberEnd()) {
      return Result<VectorNode>(
          ErrorList::error("FeatureCollection must have 'features' member."));
    }

    if (!featuresMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "FeatureCollection 'features' member must be an array of features."));
    }

    const rapidjson::Value::Array& featuresArr =
        featuresMember->value.GetArray();

    std::vector<Feature> features;
    features.reserve(featuresArr.Size());
    for (auto& feature : featuresArr) {
      if (!feature.IsObject()) {
        return Result<VectorNode>(
            ErrorList::error("FeatureCollection 'features' member must contain "
                             "only GeoJSON objects."));
      }

      Result<VectorNode> childResult = parseGeoJsonObject(feature.GetObject());
      errorList.merge(childResult.errors);
      if (!childResult.value) {
        continue;
      }

      Result<Feature> featureResult =
          std::visit(NodeToFeatureVisitor{}, std::move(*childResult.value));
      errorList.merge(featureResult.errors);
      if (!featureResult.value) {
        continue;
      }

      features.emplace_back(std::move(*featureResult.value));
    }

    return Result<VectorNode>(FeatureCollection{
        std::move(features),
        boundingBox,
        collectForeignMembers(obj, [](const std::string& k) {
          return k == "features";
        })});
  } else if (type == "GeometryCollection") {
    // Geometry collection contains zero or more geometry primitives
    const auto geometriesMember = obj.FindMember("geometries");
    if (geometriesMember == obj.MemberEnd() ||
        !geometriesMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "GeometryCollection requires array 'geometries' member."));
    }

    rapidjson::Value::Array childrenArr = geometriesMember->value.GetArray();

    std::vector<GeometryPrimitive> children;
    children.reserve(childrenArr.Size());
    for (auto& value : childrenArr) {
      if (!value.IsObject()) {
        return Result<VectorNode>(
            ErrorList::error("GeometryCollection 'geometries' member must "
                             "contain only GeoJSON objects."));
        continue;
      }

      Result<VectorNode> child = parseGeoJsonObject(value.GetObject());
      errorList.merge(child.errors);
      if (!child.value) {
        continue;
      }

      Result<GeometryPrimitive> geometryResult =
          std::visit(NodeToGeometryPrimitiveVisitor{}, std::move(*child.value));
      errorList.merge(geometryResult.errors);
      if (!geometryResult.value) {
        continue;
      }

      children.emplace_back(std::move(*geometryResult.value));
    }

    if (errorList.hasErrors()) {
      return Result<VectorNode>(errorList);
    }

    return Result<VectorNode>(
        GeometryCollection{
            std::move(children),
            boundingBox,
            collectForeignMembers(
                obj,
                [](const std::string& k) { return k == "geometries"; })},
        errorList);
  }

  // The rest of the types use a "coordinates" field, so we can fetch it up
  // front.
  const auto coordinatesMember = obj.FindMember("coordinates");
  if (coordinatesMember == obj.MemberEnd()) {
    return Result<VectorNode>(
        ErrorList::error("'coordinates' member required."));
  }

  // All of these types have only a "coordinates" property unique to them.
  JsonValue::Object foreignMembers =
      collectForeignMembers(obj, [](const std::string& k) {
        return k == "coordinates";
      });

  if (type == "Point") {
    // Point primitive has a "coordinates" member that consists of a single
    // position.
    Result<Cartographic> posResult = parsePosition(coordinatesMember->value);
    if (!posResult.value) {
      return Result<VectorNode>(std::move(posResult.errors));
    }

    return Result<VectorNode>(
        Point{std::move(*posResult.value), boundingBox, foreignMembers},
        errorList);
  } else if (type == "MultiPoint") {
    // MultiPoint primitive has a "coordinates" member that consists of an array
    // of positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "MultiPoint 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<VectorNode>(std::move(pointsResult.errors));
    }

    return Result<VectorNode>(
        MultiPoint{std::move(*pointsResult.value), boundingBox, foreignMembers},
        errorList);
  } else if (type == "LineString") {
    // LineString primitive has a "coordinates" member that consists of an array
    // of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "LineString 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<VectorNode>(std::move(pointsResult.errors));
    } else if (pointsResult.value->size() < 2) {
      return Result<VectorNode>(
          ErrorList::error("LineString 'coordinates' member must contain two "
                           "or more positions."));
    }

    return Result<VectorNode>(
        LineString{std::move(*pointsResult.value), boundingBox, foreignMembers},
        errorList);
  } else if (type == "MultiLineString") {
    // MultiLineString has a "coordinates" member that consists of an array of
    // arrays of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<VectorNode>(
          ErrorList::error("MultiLineString 'coordinates' member must be an "
                           "array of position arrays."));
    }

    const rapidjson::Value::Array coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> linesResult =
        parsePolygon(coordinatesArr, "MultiLineString", 2);
    if (!linesResult.value) {
      return Result<VectorNode>(std::move(linesResult.errors));
    }

    return Result<VectorNode>(
        MultiLineString{
            std::move(*linesResult.value),
            boundingBox,
            foreignMembers},
        errorList);
  } else if (type == "Polygon") {
    // Polygon has a "coordinates" member that consists of an array of arrays of
    // four or more positions. It's equivalent to the contents of a
    // MultiLineString, but the requirement is four positions per, not two.
    if (!coordinatesMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "Polygon 'coordinates' member must be an array of position arrays."));
    }

    const rapidjson::Value::Array coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> ringsResult =
        parsePolygon(coordinatesArr, "Polygon", 4);
    if (!ringsResult.value) {
      return Result<VectorNode>(std::move(ringsResult.errors));
    }

    return Result<VectorNode>(
        Polygon{std::move(*ringsResult.value), boundingBox, foreignMembers},
        errorList);
  } else if (type == "MultiPolygon") {
    // MultiPolygon has a "coordinates" member that consists of an array of
    // arrays of arrays of four or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<VectorNode>(
          ErrorList::error("MultiPolygon 'coordinates' member must be an array "
                           "of arrays of position arrays."));
    }

    const rapidjson::Value::Array coordinatesArr =
        coordinatesMember->value.GetArray();

    std::vector<std::vector<std::vector<Cartographic>>> polygons;
    polygons.reserve(coordinatesArr.Size());
    for (auto& value : coordinatesArr) {
      if (!value.IsArray()) {
        return Result<VectorNode>(
            ErrorList::error("MultiPolygon 'coordinates' member must be an "
                             "array of arrays of position arrays."));
      }

      Result<std::vector<std::vector<Cartographic>>> ringsResult =
          parsePolygon(value.GetArray(), "MultiPolygon", 4);
      if (!ringsResult.value) {
        return Result<VectorNode>(std::move(ringsResult.errors));
      }

      polygons.emplace_back(std::move(*ringsResult.value));
    }

    return Result<VectorNode>(
        MultiPolygon{std::move(polygons), boundingBox, foreignMembers},
        errorList);
  }

  return Result<VectorNode>(
      ErrorList::error(fmt::format("Unknown GeoJSON object type: '{}'", type)));
}
} // namespace

Result<VectorNode> parseGeoJson(const std::span<const std::byte>& bytes) {
  rapidjson::Document d;
  d.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (d.HasParseError()) {
    return Result<VectorNode>(ErrorList::error(fmt::format(
        "Failed to parse GeoJSON: {}",
        rapidjson::GetParseError_En(d.GetParseError()))));
  } else if (!d.IsObject()) {
    return Result<VectorNode>(
        ErrorList::error("GeoJSON must contain a JSON object."));
  }

  return parseGeoJsonObject(d.GetObject());
}

} // namespace CesiumVectorData