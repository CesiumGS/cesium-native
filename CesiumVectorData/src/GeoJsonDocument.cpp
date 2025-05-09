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
std::optional<ErrorList>
parsePosition(const rapidjson::Value& pos, Cartographic& outCartographic) {
  if (!pos.IsArray()) {
    return ErrorList::error("Position value must be an array.");
  }

  rapidjson::Value::ConstArray arr = pos.GetArray();

  const int32_t size = static_cast<int32_t>(arr.Size());
  if (size < 2 || size > 3) {
    return ErrorList::error(
        "Position value must be an array with two or three members.");
  }

  ErrorList notNumberError =
      ErrorList::error("Position value must be an array of only numbers.");

  // Parsing manually without an intermediate vector to avoid unnecessary
  // allocations.
  outCartographic = Cartographic(0.0, 0.0);

  if (!arr[0].IsNumber()) {
    return notNumberError;
  }

  outCartographic.longitude = Math::degreesToRadians(arr[0].GetDouble());

  if (!arr[1].IsNumber()) {
    return notNumberError;
  }

  outCartographic.latitude = Math::degreesToRadians(arr[1].GetDouble());

  if (size > 2) {
    if (!arr[2].IsNumber()) {
      return notNumberError;
    }

    outCartographic.height = arr[2].GetDouble();
  }

  return std::nullopt;
}

std::optional<ErrorList> parsePositionArray(
    const rapidjson::Value::ConstArray& arr,
    std::vector<Cartographic>& pointData,
    int32_t& outStartIndex,
    int32_t& outEndIndex) {
  outStartIndex = static_cast<int32_t>(pointData.size());
  pointData.reserve(pointData.size() + arr.Size());
  for (auto& value : arr) {
    Cartographic result = Cartographic(0.0, 0.0, 0.0);
    std::optional<ErrorList> coordinateParseError =
        parsePosition(value, result);
    if (coordinateParseError) {
      return coordinateParseError;
    }
    pointData.emplace_back(std::move(result));
  }
  outEndIndex = static_cast<int32_t>(pointData.size() - 1);

  if (outStartIndex > outEndIndex) {
    // Empty position array
    outStartIndex = outEndIndex = -1;
  }

  return std::nullopt;
}

std::optional<ErrorList> parsePolygon(
    const rapidjson::Value::ConstArray& arr,
    std::vector<GeoJsonLineString>& lineStringData,
    std::vector<Cartographic>& pointData,
    int32_t& outStartIndex,
    int32_t& outEndIndex,
    const std::string& name,
    int32_t minItems,
    bool mustBeClosed) {
  outStartIndex = static_cast<int32_t>(lineStringData.size());
  lineStringData.reserve(lineStringData.size() + arr.Size());
  for (auto& value : arr) {
    if (!value.IsArray()) {
      return ErrorList::error(
          name + " 'coordinates' member must be an array of position arrays.");
    }

    GeoJsonLineString& lineString = lineStringData.emplace_back();
    std::optional<ErrorList> pointsParseError = parsePositionArray(
        value.GetArray(),
        pointData,
        lineString.pointStartIndex,
        lineString.pointEndIndex);
    if (pointsParseError) {
      return pointsParseError;
    } else if (
        lineString.pointStartIndex == -1 || lineString.pointEndIndex == -1 ||
        (lineString.pointEndIndex - lineString.pointStartIndex + 1) <
            minItems) {
      return ErrorList::error(fmt::format(
          "{} 'coordinates' member must be an array of "
          "arrays of {} or more positions.",
          name,
          minItems));
    }

    if (mustBeClosed &&
        pointData[static_cast<size_t>(lineString.pointStartIndex)] !=
            pointData[static_cast<size_t>(lineString.pointEndIndex)]) {
      return ErrorList::error(fmt::format(
          "{} 'coordinates' member can only contain closed rings, requiring "
          "the first and last coordinates of each ring to have identical "
          "values.",
          name,
          minItems));
    }
  }

  outEndIndex = static_cast<int32_t>(lineStringData.size() - 1);
  if (outStartIndex > outEndIndex) {
    outStartIndex = outEndIndex = -1;
  }

  return std::nullopt;
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
    Cartographic position(0.0, 0.0, 0.0);
    std::optional<ErrorList> posParseError =
        parsePosition(coordinatesMember->value, position);
    if (posParseError) {
      return Result<GeoJsonObjectDescriptor>(std::move(*posParseError));
    }

    this->_pointData.emplace_back(std::move(position));
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

    std::optional<ErrorList> pointsParseError = parsePositionArray(
        coordinatesMember->value.GetArray(),
        this->_pointData,
        node.dataStartIndex,
        node.dataEndIndex);
    if (pointsParseError) {
      return Result<GeoJsonObjectDescriptor>(std::move(*pointsParseError));
    }
  } else if (type == "LineString") {
    node.type = GeoJsonObjectType::LineString;
    // LineString primitive has a "coordinates" member that consists of an array
    // of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObjectDescriptor>(ErrorList::error(
          "LineString 'coordinates' member must be an array of positions."));
    }

    GeoJsonLineString& lineString = this->_lineStringData.emplace_back();
    std::optional<ErrorList> pointsParseError = parsePositionArray(
        coordinatesMember->value.GetArray(),
        this->_pointData,
        lineString.pointStartIndex,
        lineString.pointEndIndex);
    if (pointsParseError) {
      return Result<GeoJsonObjectDescriptor>(std::move(*pointsParseError));
    } else if (
        lineString.pointStartIndex == -1 || lineString.pointEndIndex == -1 ||
        (lineString.pointEndIndex - lineString.pointStartIndex + 1) < 2) {
      return Result<GeoJsonObjectDescriptor>(
          ErrorList::error("LineString 'coordinates' member must contain two "
                           "or more positions."));
    }

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

    std::optional<ErrorList> linesParseError = parsePolygon(
        coordinatesArr,
        this->_lineStringData,
        this->_pointData,
        node.dataStartIndex,
        node.dataEndIndex,
        "MultiLineString",
        2,
        false);
    if (linesParseError) {
      return Result<GeoJsonObjectDescriptor>(std::move(*linesParseError));
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

    GeoJsonPolygon& polygon = this->_polygonData.emplace_back();
    std::optional<ErrorList> ringsParseError = parsePolygon(
        coordinatesArr,
        this->_lineStringData,
        this->_pointData,
        polygon.lineStringStartIndex,
        polygon.lineStringEndIndex,
        "Polygon",
        4,
        true);
    if (ringsParseError) {
      return Result<GeoJsonObjectDescriptor>(std::move(*ringsParseError));
    }
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

      GeoJsonPolygon& polygon = this->_polygonData.emplace_back();
      std::optional<ErrorList> ringsParseError = parsePolygon(
          value.GetArray(),
          this->_lineStringData,
          this->_pointData,
          polygon.lineStringStartIndex,
          polygon.lineStringEndIndex,
          "MultiPolygon",
          4,
          true);
      if (ringsParseError) {
        return Result<GeoJsonObjectDescriptor>(std::move(*ringsParseError));
      }
    }
    node.dataEndIndex = static_cast<int32_t>(this->_polygonData.size() - 1);

    if (node.dataStartIndex > node.dataEndIndex) {
      node.dataStartIndex = node.dataEndIndex = -1;
    }
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