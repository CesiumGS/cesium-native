#include "CesiumVectorData/GeoJsonObjectDescriptor.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
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
    const rapidjson::Value::ConstObject& obj,
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
parsePositionArray(const rapidjson::Value::ConstArray& arr) {
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
    const rapidjson::Value::ConstArray& arr,
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

void emplaceLine(
    std::vector<GeoJsonLineString>& lineStringData,
    std::vector<Cartographic>& pointData,
    std::vector<Cartographic>& linePoints) {
  GeoJsonLineString& lineString = lineStringData.emplace_back();
  lineString.pointStartIndex = static_cast<int32_t>(pointData.size());
  pointData.insert(
      pointData.end(),
      std::make_move_iterator(linePoints.begin()),
      std::make_move_iterator(linePoints.end()));
  lineString.pointEndIndex = static_cast<int32_t>(pointData.size() - 1);

  if (lineString.pointStartIndex > lineString.pointEndIndex) {
    // Empty LineString
    lineString.pointStartIndex = lineString.pointEndIndex = -1;
  }
}

void emplacePolygon(
    std::vector<GeoJsonPolygon>& polygonData,
    std::vector<GeoJsonLineString>& lineStringData,
    std::vector<Cartographic>& pointData,
    std::vector<std::vector<Cartographic>>& linePoints) {
  GeoJsonPolygon polygon = polygonData.emplace_back();
  polygon.lineStringStartIndex = static_cast<int32_t>(lineStringData.size());
  for (std::vector<Cartographic>& line : linePoints) {
    emplaceLine(lineStringData, pointData, line);
  }
  polygon.lineStringEndIndex = static_cast<int32_t>(lineStringData.size() - 1);

  if (polygon.lineStringStartIndex > polygon.lineStringEndIndex) {
    polygon.lineStringStartIndex = polygon.lineStringEndIndex = -1;
  }
}
} // namespace

Result<GeoJsonObjectDescriptor> GeoJsonDocument::parseGeoJsonObject(
    const rapidjson::Value::ConstObject& obj,
    const std::function<bool(const std::string& type)>& expectedPredicate,
    const std::string& expectedStr) {
  const std::string& type = JsonHelpers::getStringOrDefault(obj, "type", "");
  if (type.empty()) {
    return Result<GeoJsonObjectDescriptor>(
        ErrorList::error("GeoJSON object missing required 'type' field."));
  }

  if (!expectedPredicate(type)) {
    return Result<GeoJsonObjectDescriptor>(
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

  GeoJsonObjectDescriptor node;
  if (boundingBox) {
    this->_boundingBoxData.emplace_back(std::move(*boundingBox));
    node.boundingBoxIndex =
        static_cast<int32_t>(this->_boundingBoxData.size() - 1);
  }

  if (type == "Feature") {
    node.type = GeoJsonObjectType::Feature;
    GeoJsonFeature& feature = this->_featureData.emplace_back();
    node.dataStartIndex = node.dataEndIndex =
        static_cast<int32_t>(this->_featureData.size() - 1);
    // Feature has a geometry, properties, and an optional id
    const auto& idMember = obj.FindMember("id");
    if (idMember != obj.MemberEnd()) {
      if (idMember->value.IsNumber()) {
        feature.id = idMember->value.GetInt64();
      } else if (idMember->value.IsString()) {
        feature.id = idMember->value.GetString();
      } else {
        return Result<GeoJsonObjectDescriptor>(ErrorList::error(
            "Feature 'id' member must be either a string or a number."));
      }
    }

    const auto& geometryMember = obj.FindMember("geometry");
    if (geometryMember == obj.MemberEnd()) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("Feature must have 'geometry' member."));
    }

    if (!geometryMember->value.IsNull()) {
      if (!geometryMember->value.IsObject()) {
        return Result<GeoJsonObjectDescriptor>(ErrorList::error(
            "Feature 'geometry' member must be either an object or null."));
      }

      Result<GeoJsonObjectDescriptor> childResult = parseGeoJsonObject(
          geometryMember->value.GetObject(),
          [](const std::string& t) {
            return t != "Feature" && t != "FeatureCollection";
          },
          "GeoJSON Feature 'geometry' member may only contain GeoJSON Geometry "
          "objects");
      if (!childResult.value) {
        return childResult;
      }

      feature.geometry = std::move(*childResult.value);
    }

    const auto& propertiesMember = obj.FindMember("properties");
    if (propertiesMember == obj.MemberEnd()) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("Feature must have 'properties' member."));
    }

    std::optional<JsonValue::Object> properties = std::nullopt;
    if (!propertiesMember->value.IsNull()) {
      if (!propertiesMember->value.IsObject()) {
        return Result<GeoJsonObjectDescriptor>(ErrorList::error(
            "Feature 'properties' member must be either an object or null."));
      }

      feature.properties =
          JsonHelpers::toJsonValue(propertiesMember->value.GetObject())
              .getObject();
    }

    JsonValue::Object foreignMembers =
        collectForeignMembers(obj, [](const std::string& k) {
          return k == "id" || k == "geometry" || k == "properties";
        });

    if (!foreignMembers.empty()) {
      this->_foreignMemberData.emplace_back(std::move(foreignMembers));
      node.foreignMembersIndex =
          static_cast<int32_t>(this->_foreignMemberData.size() - 1);
    }

    return Result<GeoJsonObjectDescriptor>(
        std::move(node),
        std::move(errorList));
  } else if (type == "FeatureCollection") {
    node.type = GeoJsonObjectType::FeatureCollection;
    // Feature collection contains zero or more features
    const auto& featuresMember = obj.FindMember("features");
    if (featuresMember == obj.MemberEnd()) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("FeatureCollection must have 'features' member."));
    }

    if (!featuresMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "FeatureCollection 'features' member must be an array of features."));
    }

    const rapidjson::Value::ConstArray& featuresArr =
        featuresMember->value.GetArray();

    node.dataStartIndex = static_cast<int32_t>(this->_featureData.size());
    this->_featureData.reserve(this->_featureData.size() + featuresArr.Size());
    for (auto& feature : featuresArr) {
      if (!feature.IsObject()) {
        return Result<GeoJsonObjectDescriptor>(
            ErrorList::error("FeatureCollection 'features' member must contain "
                             "only GeoJSON objects."));
      }

      Result<GeoJsonObjectDescriptor> childResult = parseGeoJsonObject(
          feature.GetObject(),
          [](const std::string& t) { return t == "Feature"; },
          "GeoJSON FeatureCollection 'features' member may only contain "
          "Feature objects");
      errorList.merge(childResult.errors);
      if (!childResult.value) {
        continue;
      }

      CESIUM_ASSERT(childResult.value->type == GeoJsonObjectType::Feature);
    }

    node.dataEndIndex = static_cast<int32_t>(this->_featureData.size() - 1);

    if (node.dataStartIndex > node.dataEndIndex) {
      // No data actually written, empty collection
      node.dataStartIndex = node.dataEndIndex = -1;
    }

    if (errorList.hasErrors()) {
      return Result<GeoJsonObjectDescriptor>(errorList);
    }

    JsonValue::Object foreignMembers =
        collectForeignMembers(obj, [](const std::string& k) {
          return k == "features";
        });

    if (!foreignMembers.empty()) {
      this->_foreignMemberData.emplace_back(std::move(foreignMembers));
      node.foreignMembersIndex =
          static_cast<int32_t>(this->_foreignMemberData.size() - 1);
    }

    return Result<GeoJsonObjectDescriptor>(
        std::move(node),
        std::move(errorList));
  } else if (type == "GeometryCollection") {
    node.type = GeoJsonObjectType::GeometryCollection;
    // Geometry collection contains zero or more geometry primitives
    const auto geometriesMember = obj.FindMember("geometries");
    if (geometriesMember == obj.MemberEnd() ||
        !geometriesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "GeometryCollection requires array 'geometries' member."));
    }

    rapidjson::Value::ConstArray childrenArr =
        geometriesMember->value.GetArray();

    this->_geometryData.reserve(
        this->_geometryData.size() + childrenArr.Size());
    node.dataStartIndex = static_cast<int32_t>(this->_geometryData.size());
    for (auto& value : childrenArr) {
      if (!value.IsObject()) {
        return Result<GeoJsonObjectDescriptor>(
            ErrorList::error("GeometryCollection 'geometries' member must "
                             "contain only GeoJSON objects."));
        continue;
      }

      Result<GeoJsonObjectDescriptor> child = parseGeoJsonObject(
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

      this->_geometryData.emplace_back(std::move(*child.value));
    }

    node.dataEndIndex = static_cast<int32_t>(this->_geometryData.size() - 1);

    if (node.dataStartIndex > node.dataEndIndex) {
      // No data actually written, empty collection
      node.dataStartIndex = node.dataEndIndex = -1;
    }

    if (errorList.hasErrors()) {
      return Result<GeoJsonObjectDescriptor>(errorList);
    }

    JsonValue::Object foreignMembers =
        collectForeignMembers(obj, [](const std::string& k) {
          return k == "geometries";
        });

    if (!foreignMembers.empty()) {
      this->_foreignMemberData.emplace_back(std::move(foreignMembers));
      node.foreignMembersIndex =
          static_cast<int32_t>(this->_foreignMemberData.size() - 1);
    }

    return Result<GeoJsonObjectDescriptor>(
        std::move(node),
        std::move(errorList));
  }

  // The rest of the types use a "coordinates" field, so we can fetch it up
  // front.
  const auto coordinatesMember = obj.FindMember("coordinates");
  if (coordinatesMember == obj.MemberEnd()) {
    return Result<GeoJsonObjectDescriptor>(
        ErrorList::error("'coordinates' member required."));
  }

  // All of these types have only a "coordinates" property unique to them.
  JsonValue::Object foreignMembers =
      collectForeignMembers(obj, [](const std::string& k) {
        return k == "coordinates";
      });

  if (!foreignMembers.empty()) {
    this->_foreignMemberData.emplace_back(std::move(foreignMembers));
    node.foreignMembersIndex =
        static_cast<int32_t>(this->_foreignMemberData.size() - 1);
  }

  if (type == "Point") {
    node.type = GeoJsonObjectType::Point;
    // Point primitive has a "coordinates" member that consists of a single
    // position.
    Result<Cartographic> posResult = parsePosition(coordinatesMember->value);
    if (!posResult.value) {
      return Result<GeoJsonObjectDescriptor>(std::move(posResult.errors));
    }

    this->_pointData.emplace_back(std::move(*posResult.value));
    node.dataStartIndex = node.dataEndIndex =
        static_cast<int32_t>(this->_pointData.size() - 1);
  } else if (type == "MultiPoint") {
    node.type = GeoJsonObjectType::MultiPoint;
    // MultiPoint primitive has a "coordinates" member that consists of an array
    // of positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "MultiPoint 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<GeoJsonObjectDescriptor>(std::move(pointsResult.errors));
    }

    node.dataStartIndex = static_cast<int32_t>(this->_pointData.size());
    this->_pointData.insert(
        this->_pointData.end(),
        std::make_move_iterator(pointsResult.value->begin()),
        std::make_move_iterator(pointsResult.value->end()));
    node.dataEndIndex = static_cast<int32_t>(this->_pointData.size() - 1);

    if (node.dataStartIndex > node.dataEndIndex) {
      // Empty MultiPoint
      node.dataStartIndex = node.dataEndIndex = -1;
    }
  } else if (type == "LineString") {
    node.type = GeoJsonObjectType::LineString;
    // LineString primitive has a "coordinates" member that consists of an array
    // of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "LineString 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<GeoJsonObjectDescriptor>(std::move(pointsResult.errors));
    } else if (pointsResult.value->size() < 2) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("LineString 'coordinates' member must contain two "
                           "or more positions."));
    }

    emplaceLine(this->_lineStringData, this->_pointData, *pointsResult.value);

    node.dataStartIndex = node.dataEndIndex =
        static_cast<int32_t>(this->_lineStringData.size() - 1);
  } else if (type == "MultiLineString") {
    node.type = GeoJsonObjectType::MultiLineString;
    // MultiLineString has a "coordinates" member that consists of an array of
    // arrays of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("MultiLineString 'coordinates' member must be an "
                           "array of position arrays."));
    }

    const rapidjson::Value::ConstArray coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> linesResult =
        parsePolygon(coordinatesArr, "MultiLineString", 2, false);
    if (!linesResult.value) {
      return Result<GeoJsonObjectDescriptor>(std::move(linesResult.errors));
    }

    this->_lineStringData.reserve(
        this->_lineStringData.size() + linesResult.value->size());
    node.dataStartIndex = static_cast<int32_t>(this->_lineStringData.size());
    for (std::vector<CesiumGeospatial::Cartographic>& line :
         *linesResult.value) {
      emplaceLine(this->_lineStringData, this->_pointData, line);
    }
    node.dataEndIndex = static_cast<int32_t>(this->_lineStringData.size() - 1);

    if (node.dataStartIndex > node.dataEndIndex) {
      node.dataStartIndex = node.dataEndIndex = -1;
    }
  } else if (type == "Polygon") {
    node.type = GeoJsonObjectType::Polygon;
    // Polygon has a "coordinates" member that consists of an array of arrays of
    // four or more positions. It's equivalent to the contents of a
    // MultiLineString, but the requirement is four positions per, not two.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "Polygon 'coordinates' member must be an array of position arrays."));
    }

    const rapidjson::Value::ConstArray coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> ringsResult =
        parsePolygon(coordinatesArr, "Polygon", 4, true);
    if (!ringsResult.value) {
      return Result<GeoJsonObjectDescriptor>(std::move(ringsResult.errors));
    }

    emplacePolygon(
        this->_polygonData,
        this->_lineStringData,
        this->_pointData,
        *ringsResult.value);
    node.dataStartIndex = node.dataEndIndex =
        static_cast<int32_t>(this->_polygonData.size() - 1);
  } else if (type == "MultiPolygon") {
    node.type = GeoJsonObjectType::MultiPolygon;
    // MultiPolygon has a "coordinates" member that consists of an array of
    // arrays of arrays of four or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("MultiPolygon 'coordinates' member must be an array "
                           "of arrays of position arrays."));
    }

    const rapidjson::Value::ConstArray coordinatesArr =
        coordinatesMember->value.GetArray();

    this->_polygonData.reserve(
        this->_polygonData.size() + coordinatesArr.Size());
    node.dataStartIndex = static_cast<int32_t>(this->_polygonData.size());
    for (auto& value : coordinatesArr) {
      if (!value.IsArray()) {
        return Result<GeoJsonObjectDescriptor>(
            ErrorList::error("MultiPolygon 'coordinates' member must be an "
                             "array of arrays of position arrays."));
      }

      Result<std::vector<std::vector<Cartographic>>> ringsResult =
          parsePolygon(value.GetArray(), "MultiPolygon", 4, true);
      if (!ringsResult.value) {
        return Result<GeoJsonObjectDescriptor>(std::move(ringsResult.errors));
      }

      emplacePolygon(
          this->_polygonData,
          this->_lineStringData,
          this->_pointData,
          *ringsResult.value);
    }
    node.dataEndIndex = static_cast<int32_t>(this->_polygonData.size() - 1);
  } else {
    return Result<GeoJsonObjectDescriptor>(ErrorList::error(
        fmt::format("Unknown GeoJSON object type: '{}'", type)));
  }

  return Result<GeoJsonObjectDescriptor>(std::move(node), errorList);
}

Result<GeoJsonObjectDescriptor>
GeoJsonDocument::parseGeoJson(const rapidjson::Document& doc) {
  if (!doc.IsObject()) {
    return Result<GeoJsonObjectDescriptor>(
        ErrorList::error("GeoJSON must contain a JSON object."));
  }

  return parseGeoJsonObject(
      doc.GetObject(),
      [](const std::string&) { return true; },
      "");
}

Result<GeoJsonObjectDescriptor>
GeoJsonDocument::parseGeoJson(const std::span<const std::byte>& bytes) {
  rapidjson::Document d;
  d.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (d.HasParseError()) {
    return Result<GeoJsonObjectDescriptor>(ErrorList::error(fmt::format(
        "Failed to parse GeoJSON: {} at offset {}",
        rapidjson::GetParseError_En(d.GetParseError()),
        d.GetErrorOffset())));
  }

  return parseGeoJson(d);
}

Result<IntrusivePointer<GeoJsonDocument>> GeoJsonDocument::fromGeoJson(
    const std::span<const std::byte>& bytes,
    std::vector<VectorDocumentAttribution>&& attributions) {
  IntrusivePointer<GeoJsonDocument> pDocument;
  GeoJsonDocument& document = pDocument.emplace();
  document._attributions = std::move(attributions);
  Result<GeoJsonObjectDescriptor> parseResult = document.parseGeoJson(bytes);
  if (!parseResult.value) {
    return Result<IntrusivePointer<GeoJsonDocument>>(
        std::move(parseResult.errors));
  }

  document._rootObject = std::move(*parseResult.value);

  return Result<IntrusivePointer<GeoJsonDocument>>(
      pDocument,
      std::move(parseResult.errors));
}

Result<IntrusivePointer<GeoJsonDocument>> GeoJsonDocument::fromGeoJson(
    const rapidjson::Document& document,
    std::vector<VectorDocumentAttribution>&& attributions) {
  IntrusivePointer<GeoJsonDocument> pDocument;
  GeoJsonDocument& geoJsonDocument = pDocument.emplace();
  geoJsonDocument._attributions = std::move(attributions);
  Result<GeoJsonObjectDescriptor> parseResult =
      geoJsonDocument.parseGeoJson(document);
  if (!parseResult.value) {
    return Result<IntrusivePointer<GeoJsonDocument>>(
        std::move(parseResult.errors));
  }

  geoJsonDocument._rootObject = std::move(*parseResult.value);

  return Result<IntrusivePointer<GeoJsonDocument>>(
      pDocument,
      std::move(parseResult.errors));
}

Future<Result<IntrusivePointer<GeoJsonDocument>>>
GeoJsonDocument::fromCesiumIonAsset(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl) {
  const std::string url = fmt::format(
      "{}v1/assets/{}/endpoint?access_token={}",
      ionAssetEndpointUrl,
      ionAssetID,
      ionAccessToken);
  return pAssetAccessor->get(asyncSystem, url)
      .thenImmediately([asyncSystem, pAssetAccessor](
                           std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          return asyncSystem
              .createResolvedFuture<Result<IntrusivePointer<GeoJsonDocument>>>(
                  Result<IntrusivePointer<GeoJsonDocument>>(
                      ErrorList::error(fmt::format(
                          "Status code {} while requesting Cesium ion vector "
                          "asset.",
                          pResponse->statusCode()))));
        }

        rapidjson::Document response;
        response.Parse(
            reinterpret_cast<const char*>(pResponse->data().data()),
            pResponse->data().size());

        if (response.HasParseError()) {
          return asyncSystem
              .createResolvedFuture<Result<IntrusivePointer<GeoJsonDocument>>>(
                  Result<IntrusivePointer<GeoJsonDocument>>(
                      ErrorList::error(fmt::format(
                          "Error while parsing Cesium ion asset response: "
                          "error {} at byte offset {}.",
                          rapidjson::GetParseError_En(response.GetParseError()),
                          response.GetErrorOffset()))));
        }

        const std::string type =
            JsonHelpers::getStringOrDefault(response, "type", "UNKNOWN");
        if (type != "GEOJSON") {
          return asyncSystem
              .createResolvedFuture<Result<IntrusivePointer<GeoJsonDocument>>>(
                  Result<IntrusivePointer<GeoJsonDocument>>(
                      ErrorList::error(fmt::format(
                          "Found asset type '{}'. Only GEOJSON is currently "
                          "supported.",
                          type))));
        }

        std::vector<VectorDocumentAttribution> attributions;

        const auto attributionsMember = response.FindMember("attributions");
        if (attributionsMember != response.MemberEnd() &&
            attributionsMember->value.IsArray()) {
          for (const rapidjson::Value& attribution :
               attributionsMember->value.GetArray()) {
            VectorDocumentAttribution& docAttribution =
                attributions.emplace_back();
            docAttribution.html =
                JsonHelpers::getStringOrDefault(attribution, "html", "");
            docAttribution.showOnScreen = !JsonHelpers::getBoolOrDefault(
                attribution,
                "collapsible",
                true);
          }
        }

        const std::string assetUrl =
            JsonHelpers::getStringOrDefault(response, "url", "");
        const std::string accessToken =
            JsonHelpers::getStringOrDefault(response, "accessToken", "");

        const std::vector<IAssetAccessor::THeader> headers{
            {"Authorization", "Bearer " + accessToken}};
        return pAssetAccessor->get(asyncSystem, assetUrl, headers)
            .thenImmediately(
                [attributions = std::move(attributions)](
                    std::shared_ptr<IAssetRequest>&& pAssetRequest) mutable
                -> Result<IntrusivePointer<GeoJsonDocument>> {
                  const IAssetResponse* pAssetResponse =
                      pAssetRequest->response();

                  if (pAssetResponse->statusCode() < 200 ||
                      pAssetResponse->statusCode() >= 300) {
                    return Result<IntrusivePointer<GeoJsonDocument>>(
                        ErrorList::error(fmt::format(
                            "Status code {} while requesting Cesium ion "
                            "vector asset data.",
                            pAssetResponse->statusCode())));
                  }

                  return GeoJsonDocument::fromGeoJson(
                      pAssetResponse->data(),
                      std::move(attributions));
                });
      });
}

GeoJsonDocument::GeoJsonDocument(
    GeoJsonObjectDescriptor&& rootObject,
    std::vector<VectorDocumentAttribution>&& attributions)
    : _rootObject(std::move(rootObject)),
      _attributions(std::move(attributions)) {}
} // namespace CesiumVectorData