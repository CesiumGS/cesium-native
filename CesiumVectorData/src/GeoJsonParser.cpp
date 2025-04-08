#include "GeoJsonParser.h"

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorNode.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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
  if (!pos.IsArray()) {
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
    uint32_t minItems,
    bool mustBeClosed) {
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

    if (mustBeClosed &&
        (*pointsResult.value)[0] !=
            (*pointsResult.value)[pointsResult.value->size() - 1]) {
      return Result<
          std::vector<std::vector<Cartographic>>>(ErrorList::error(fmt::format(
          "{} 'coordinates' member can only contain closed rings, requiring "
          "the first and last coordinates of each ring to have identical "
          "values.",
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
        0,
        // GeoJSON explicitly only supports the WGS84 ellipsoid.
        Ellipsoid::WGS84));
  }

  return Result<std::optional<BoundingRegion>>(BoundingRegion(
      GlobeRectangle(
          Math::degreesToRadians((*doubles)[0]),
          Math::degreesToRadians((*doubles)[1]),
          Math::degreesToRadians((*doubles)[3]),
          Math::degreesToRadians((*doubles)[4])),
      (*doubles)[2],
      (*doubles)[5],
      Ellipsoid::WGS84));
}

Result<VectorNode> parseGeoJsonObject(
    const rapidjson::Value::Object& obj,
    const std::function<bool(const std::string& type)>& expectedPredicate,
    const std::string& expectedStr) {
  const std::string& type = JsonHelpers::getStringOrDefault(obj, "type", "");
  if (type.empty()) {
    return Result<VectorNode>(
        ErrorList::error("GeoJSON object missing required 'type' field."));
  }

  if (!expectedPredicate(type)) {
    return Result<VectorNode>(
        ErrorList::error(fmt::format("{}, found {}.", expectedStr, type)));
  }

  ErrorList errorList;
  // Try reading the bounding box
  std::optional<BoundingRegion> boundingBox;
  const auto bboxMember = obj.FindMember("bbox");
  if (bboxMember != obj.MemberEnd()) {
    Result<std::optional<BoundingRegion>> regionResult =
        parseBoundingBox(bboxMember->value);
    errorList.merge(regionResult.errors);
    if (regionResult.value) {
      boundingBox = std::move(*regionResult.value);
    }
  }

  VectorNode node;
  node.boundingBox = std::move(boundingBox);

  if (type == "Feature") {
    // Feature has a geometry, properties, and an optional id
    const auto& idMember = obj.FindMember("id");
    if (idMember != obj.MemberEnd()) {
      if (idMember->value.IsNumber()) {
        node.id = idMember->value.GetInt64();
      } else if (idMember->value.IsString()) {
        node.id = idMember->value.GetString();
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

    if (!geometryMember->value.IsNull()) {
      if (!geometryMember->value.IsObject()) {
        return Result<VectorNode>(ErrorList::error(
            "Feature 'geometry' member must be either an object or null."));
      }

      Result<VectorNode> childResult = parseGeoJsonObject(
          geometryMember->value.GetObject(),
          [](const std::string& t) {
            return t != "Feature" && t != "FeatureCollection";
          },
          "GeoJSON Feature 'geometry' member may only contain GeoJSON Geometry "
          "objects");
      if (!childResult.value) {
        return childResult;
      }

      node.children.emplace_back(std::move(*childResult.value));
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

      node.properties =
          JsonHelpers::toJsonValue(propertiesMember->value.GetObject())
              .getObject();
    }

    node.foreignMembers = collectForeignMembers(obj, [](const std::string& k) {
      return k == "id" || k == "geometry" || k == "properties";
    });

    return Result<VectorNode>(std::move(node), std::move(errorList));
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

    node.children.reserve(featuresArr.Size());
    for (auto& feature : featuresArr) {
      if (!feature.IsObject()) {
        return Result<VectorNode>(
            ErrorList::error("FeatureCollection 'features' member must contain "
                             "only GeoJSON objects."));
      }

      Result<VectorNode> childResult = parseGeoJsonObject(
          feature.GetObject(),
          [](const std::string& t) { return t == "Feature"; },
          "GeoJSON FeatureCollection 'features' member may only contain "
          "Feature objects");
      errorList.merge(childResult.errors);
      if (!childResult.value) {
        continue;
      }

      node.children.emplace_back(std::move(*childResult.value));
    }

    if (errorList.hasErrors()) {
      return Result<VectorNode>(errorList);
    }

    node.foreignMembers = collectForeignMembers(obj, [](const std::string& k) {
      return k == "features";
    });

    return Result<VectorNode>(std::move(node), std::move(errorList));
  } else if (type == "GeometryCollection") {
    // Geometry collection contains zero or more geometry primitives
    const auto geometriesMember = obj.FindMember("geometries");
    if (geometriesMember == obj.MemberEnd() ||
        !geometriesMember->value.IsArray()) {
      return Result<VectorNode>(ErrorList::error(
          "GeometryCollection requires array 'geometries' member."));
    }

    rapidjson::Value::Array childrenArr = geometriesMember->value.GetArray();

    node.children.reserve(childrenArr.Size());
    for (auto& value : childrenArr) {
      if (!value.IsObject()) {
        return Result<VectorNode>(
            ErrorList::error("GeometryCollection 'geometries' member must "
                             "contain only GeoJSON objects."));
        continue;
      }

      Result<VectorNode> child = parseGeoJsonObject(
          value.GetObject(),
          [](const std::string& t) {
            return t != "Feature" && t != "FeatureCollection";
          },
          "GeoJSON GeometryCollection 'geometries' member may only contain "
          "GeoJSON Geometry objects");
      errorList.merge(child.errors);
      if (!child.value) {
        continue;
      }

      node.children.emplace_back(std::move(*child.value));
    }

    if (errorList.hasErrors()) {
      return Result<VectorNode>(errorList);
    }

    node.foreignMembers = collectForeignMembers(obj, [](const std::string& k) {
      return k == "geometries";
    });

    return Result<VectorNode>(std::move(node), std::move(errorList));
  }

  // The rest of the types use a "coordinates" field, so we can fetch it up
  // front.
  const auto coordinatesMember = obj.FindMember("coordinates");
  if (coordinatesMember == obj.MemberEnd()) {
    return Result<VectorNode>(
        ErrorList::error("'coordinates' member required."));
  }

  // All of these types have only a "coordinates" property unique to them.
  node.foreignMembers = collectForeignMembers(obj, [](const std::string& k) {
    return k == "coordinates";
  });

  if (type == "Point") {
    // Point primitive has a "coordinates" member that consists of a single
    // position.
    Result<Cartographic> posResult = parsePosition(coordinatesMember->value);
    if (!posResult.value) {
      return Result<VectorNode>(std::move(posResult.errors));
    }

    node.primitives.emplace_back(std::move(*posResult.value));
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

    node.primitives.reserve(pointsResult.value->size());
    for (Cartographic& point : *pointsResult.value) {
      node.primitives.emplace_back(std::move(point));
    }
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

    node.primitives.emplace_back(std::move(*pointsResult.value));
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
        parsePolygon(coordinatesArr, "MultiLineString", 2, false);
    if (!linesResult.value) {
      return Result<VectorNode>(std::move(linesResult.errors));
    }

    node.primitives.reserve(linesResult.value->size());
    for (std::vector<CesiumGeospatial::Cartographic>& line :
         *linesResult.value) {
      node.primitives.emplace_back(std::move(line));
    }
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
        parsePolygon(coordinatesArr, "Polygon", 4, true);
    if (!ringsResult.value) {
      return Result<VectorNode>(std::move(ringsResult.errors));
    }

    std::vector<CartographicPolygon> rings;
    rings.reserve(ringsResult.value->size());
    for (const std::vector<Cartographic>& ring : *ringsResult.value) {
      rings.emplace_back(ring);
    }

    node.primitives.emplace_back(
        CompositeCartographicPolygon(std::move(rings)));
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

    node.primitives.reserve(coordinatesArr.Size());
    for (auto& value : coordinatesArr) {
      if (!value.IsArray()) {
        return Result<VectorNode>(
            ErrorList::error("MultiPolygon 'coordinates' member must be an "
                             "array of arrays of position arrays."));
      }

      Result<std::vector<std::vector<Cartographic>>> ringsResult =
          parsePolygon(value.GetArray(), "MultiPolygon", 4, true);
      if (!ringsResult.value) {
        return Result<VectorNode>(std::move(ringsResult.errors));
      }

      std::vector<CartographicPolygon> rings;
      rings.reserve(ringsResult.value->size());
      for (const std::vector<Cartographic>& ring : *ringsResult.value) {
        rings.emplace_back(ring);
      }

      node.primitives.emplace_back(
          CompositeCartographicPolygon(std::move(rings)));
    }
  } else {
    return Result<VectorNode>(ErrorList::error(
        fmt::format("Unknown GeoJSON object type: '{}'", type)));
  }

  return Result<VectorNode>(node, errorList);
}
} // namespace

Result<VectorNode> parseGeoJson(const std::span<const std::byte>& bytes) {
  rapidjson::Document d;
  d.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (d.HasParseError()) {
    return Result<VectorNode>(ErrorList::error(fmt::format(
        "Failed to parse GeoJSON: {} at offset {}",
        rapidjson::GetParseError_En(d.GetParseError()),
        d.GetErrorOffset())));
  } else if (!d.IsObject()) {
    return Result<VectorNode>(
        ErrorList::error("GeoJSON must contain a JSON object."));
  }

  return parseGeoJsonObject(
      d.GetObject(),
      [](const std::string&) { return true; },
      "");
}

} // namespace CesiumVectorData