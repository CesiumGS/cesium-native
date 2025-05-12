#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/GlobeRectangle.h"

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

struct GeoJsonObjectToGeoJsonGeometryObjectVisitor {
  Result<GeoJsonGeometryObject> operator()(GeoJsonPoint&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonMultiPoint&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonLineString&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonMultiLineString&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonPolygon&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonMultiPolygon&& node) {
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonGeometryCollection&& node) {
    // The specification says that implementations should avoid nesting a
    // GeometryCollection inside a GeometryCollection. However, I don't think it
    // costs us anything to handle it even if the data is ill-advised.
    return Result<GeoJsonGeometryObject>(std::move(node));
  }
  Result<GeoJsonGeometryObject> operator()(GeoJsonFeature&& /*node*/) {
    return Result<GeoJsonGeometryObject>(
        ErrorList::error("Expected geometry, found GeoJSON object Feature."));
  }
  Result<GeoJsonGeometryObject>
  operator()(GeoJsonFeatureCollection&& /*node*/) {
    return Result<GeoJsonGeometryObject>(ErrorList::error(
        "Expected geometry, found GeoJSON object FeatureCollection."));
  }
};

struct GeoJsonObjectToFeatureVisitor {
  Result<GeoJsonFeature> operator()(GeoJsonFeature&& node) {
    return Result<GeoJsonFeature>(std::move(node));
  }

  Result<GeoJsonFeature> operator()(auto&& node) {
    return Result<GeoJsonFeature>(ErrorList::error(fmt::format(
        "Expected Feature, found GeoJSON object {}.",
        geoJsonObjectTypeToString(node.type))));
  }
};

Result<GeoJsonObject> parseGeoJsonObject(
    const rapidjson::Value::ConstObject& obj,
    const std::function<bool(const std::string& type)>& expectedPredicate,
    const std::string& expectedStr) {
  const std::string& type = JsonHelpers::getStringOrDefault(obj, "type", "");
  if (type.empty()) {
    return Result<GeoJsonObject>(
        ErrorList::error("GeoJSON object missing required 'type' field."));
  }

  // Handle unexpected types before we go through the work of parsing something
  // we won't use.
  if (!expectedPredicate(type)) {
    return Result<GeoJsonObject>(
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

  if (type == "Feature") {
    // Feature has a geometry, properties, and an optional id
    std::variant<std::string, int64_t, std::monostate> id = std::monostate();
    const auto& idMember = obj.FindMember("id");
    if (idMember != obj.MemberEnd()) {
      if (idMember->value.IsNumber()) {
        id = idMember->value.GetInt64();
      } else if (idMember->value.IsString()) {
        id = idMember->value.GetString();
      } else {
        return Result<GeoJsonObject>(ErrorList::error(
            "Feature 'id' member must be either a string or a number."));
      }
    }

    const auto& geometryMember = obj.FindMember("geometry");
    if (geometryMember == obj.MemberEnd()) {
      return Result<GeoJsonObject>(
          ErrorList::error("Feature must have 'geometry' member."));
    }

    std::optional<GeoJsonGeometryObject> geometry = std::nullopt;
    if (!geometryMember->value.IsNull()) {
      if (!geometryMember->value.IsObject()) {
        return Result<GeoJsonObject>(ErrorList::error(
            "Feature 'geometry' member must be either an object or null."));
      }

      Result<GeoJsonObject> childResult = parseGeoJsonObject(
          geometryMember->value.GetObject(),
          [](const std::string& t) {
            return t != "Feature" && t != "FeatureCollection";
          },
          "GeoJSON Feature 'geometry' member may only contain GeoJSON Geometry "
          "objects");
      if (!childResult.value) {
        return childResult;
      }

      Result<GeoJsonGeometryObject> geometryResult = std::visit(
          GeoJsonObjectToGeoJsonGeometryObjectVisitor{},
          std::move(*childResult.value));
      if (!geometryResult.value) {
        return Result<GeoJsonObject>(std::move(geometryResult.errors));
      }

      geometry = std::move(*geometryResult.value);
    }

    const auto& propertiesMember = obj.FindMember("properties");
    if (propertiesMember == obj.MemberEnd()) {
      return Result<GeoJsonObject>(
          ErrorList::error("Feature must have 'properties' member."));
    }

    std::optional<JsonValue::Object> properties = std::nullopt;
    if (!propertiesMember->value.IsNull()) {
      if (!propertiesMember->value.IsObject()) {
        return Result<GeoJsonObject>(ErrorList::error(
            "Feature 'properties' member must be either an object or null."));
      }

      properties = JsonHelpers::toJsonValue(propertiesMember->value.GetObject())
                       .getObject();
    }

    return Result<GeoJsonObject>(
        GeoJsonFeature{
            id,
            geometry,
            properties,
            boundingBox,
            collectForeignMembers(
                obj,
                [](const std::string& k) {
                  return k == "id" || k == "geometry" || k == "properties";
                })},
        errorList);
  } else if (type == "FeatureCollection") {
    // Feature collection contains zero or more features
    const auto& featuresMember = obj.FindMember("features");
    if (featuresMember == obj.MemberEnd()) {
      return Result<GeoJsonObject>(
          ErrorList::error("FeatureCollection must have 'features' member."));
    }

    if (!featuresMember->value.IsArray()) {
      return Result<GeoJsonObject>(ErrorList::error(
          "FeatureCollection 'features' member must be an array of features."));
    }

    const rapidjson::Value::ConstArray& featuresArr =
        featuresMember->value.GetArray();

    std::vector<GeoJsonFeature> features;
    features.reserve(featuresArr.Size());
    for (auto& feature : featuresArr) {
      if (!feature.IsObject()) {
        return Result<GeoJsonObject>(
            ErrorList::error("FeatureCollection 'features' member must contain "
                             "only GeoJSON objects."));
      }

      Result<GeoJsonObject> childResult = parseGeoJsonObject(
          feature.GetObject(),
          [](const std::string& t) { return t == "Feature"; },
          "GeoJSON FeatureCollection 'features' member may only contain "
          "Feature objects");
      errorList.merge(childResult.errors);
      if (!childResult.value) {
        continue;
      }

      Result<GeoJsonFeature> featureResult = std::visit(
          GeoJsonObjectToFeatureVisitor{},
          std::move(*childResult.value));
      errorList.merge(featureResult.errors);
      if (!featureResult.value) {
        continue;
      }

      features.emplace_back(std::move(*featureResult.value));
    }

    if (errorList.hasErrors()) {
      return Result<GeoJsonObject>(errorList);
    }

    return Result<GeoJsonObject>(
        GeoJsonFeatureCollection{
            std::move(features),
            boundingBox,
            collectForeignMembers(
                obj,
                [](const std::string& k) { return k == "features"; })},
        errorList);
  } else if (type == "GeometryCollection") {
    // Geometry collection contains zero or more geometry primitives
    const auto geometriesMember = obj.FindMember("geometries");
    if (geometriesMember == obj.MemberEnd() ||
        !geometriesMember->value.IsArray()) {
      return Result<GeoJsonObject>(ErrorList::error(
          "GeometryCollection requires array 'geometries' member."));
    }

    const rapidjson::Value::ConstArray& childrenArr =
        geometriesMember->value.GetArray();

    std::vector<GeoJsonGeometryObject> children;
    children.reserve(childrenArr.Size());
    for (auto& value : childrenArr) {
      if (!value.IsObject()) {
        return Result<GeoJsonObject>(
            ErrorList::error("GeometryCollection 'geometries' member must "
                             "contain only GeoJSON objects."));
        continue;
      }

      Result<GeoJsonObject> child = parseGeoJsonObject(
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

      Result<GeoJsonGeometryObject> geometryResult = std::visit(
          GeoJsonObjectToGeoJsonGeometryObjectVisitor{},
          std::move(*child.value));
      errorList.merge(geometryResult.errors);
      if (!geometryResult.value) {
        continue;
      }

      children.emplace_back(std::move(*geometryResult.value));
    }

    if (errorList.hasErrors()) {
      return Result<GeoJsonObject>(errorList);
    }

    return Result<GeoJsonObject>(
        GeoJsonGeometryCollection{
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
    return Result<GeoJsonObject>(
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
      return Result<GeoJsonObject>(std::move(posResult.errors));
    }

    return Result<GeoJsonObject>(
        GeoJsonPoint{std::move(*posResult.value), boundingBox, foreignMembers},
        errorList);
  } else if (type == "MultiPoint") {
    // MultiPoint primitive has a "coordinates" member that consists of an array
    // of positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObject>(ErrorList::error(
          "MultiPoint 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<GeoJsonObject>(std::move(pointsResult.errors));
    }

    return Result<GeoJsonObject>(
        GeoJsonMultiPoint{
            std::move(*pointsResult.value),
            boundingBox,
            foreignMembers},
        errorList);
  } else if (type == "LineString") {
    // LineString primitive has a "coordinates" member that consists of an array
    // of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObject>(ErrorList::error(
          "LineString 'coordinates' member must be an array of positions."));
    }

    Result<std::vector<Cartographic>> pointsResult =
        parsePositionArray(coordinatesMember->value.GetArray());
    if (!pointsResult.value) {
      return Result<GeoJsonObject>(std::move(pointsResult.errors));
    } else if (pointsResult.value->size() < 2) {
      return Result<GeoJsonObject>(
          ErrorList::error("LineString 'coordinates' member must contain two "
                           "or more positions."));
    }

    return Result<GeoJsonObject>(
        GeoJsonLineString{
            std::move(*pointsResult.value),
            boundingBox,
            foreignMembers},
        errorList);
  } else if (type == "MultiLineString") {
    // MultiLineString has a "coordinates" member that consists of an array of
    // arrays of two or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObject>(
          ErrorList::error("MultiLineString 'coordinates' member must be an "
                           "array of position arrays."));
    }

    const rapidjson::Value::ConstArray& coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> linesResult =
        parsePolygon(coordinatesArr, "MultiLineString", 2, false);
    if (!linesResult.value) {
      return Result<GeoJsonObject>(std::move(linesResult.errors));
    }

    return Result<GeoJsonObject>(
        GeoJsonMultiLineString{
            std::move(*linesResult.value),
            boundingBox,
            foreignMembers},
        errorList);
  } else if (type == "Polygon") {
    // Polygon has a "coordinates" member that consists of an array of arrays of
    // four or more positions. It's equivalent to the contents of a
    // MultiLineString, but the requirement is four positions per, not two.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObject>(ErrorList::error(
          "Polygon 'coordinates' member must be an array of position arrays."));
    }

    const rapidjson::Value::ConstArray& coordinatesArr =
        coordinatesMember->value.GetArray();

    Result<std::vector<std::vector<Cartographic>>> ringsResult =
        parsePolygon(coordinatesArr, "Polygon", 4, true);
    if (!ringsResult.value) {
      return Result<GeoJsonObject>(std::move(ringsResult.errors));
    }

    return Result<GeoJsonObject>(
        GeoJsonPolygon{
            std::move(*ringsResult.value),
            boundingBox,
            foreignMembers},
        errorList);
  } else if (type == "MultiPolygon") {
    // MultiPolygon has a "coordinates" member that consists of an array of
    // arrays of arrays of four or more positions.
    if (!coordinatesMember->value.IsArray()) {
      return Result<GeoJsonObject>(
          ErrorList::error("MultiPolygon 'coordinates' member must be an array "
                           "of arrays of position arrays."));
    }

    const rapidjson::Value::ConstArray& coordinatesArr =
        coordinatesMember->value.GetArray();

    std::vector<std::vector<std::vector<Cartographic>>> polygons;
    polygons.reserve(coordinatesArr.Size());
    for (auto& value : coordinatesArr) {
      if (!value.IsArray()) {
        return Result<GeoJsonObject>(
            ErrorList::error("MultiPolygon 'coordinates' member must be an "
                             "array of arrays of position arrays."));
      }

      Result<std::vector<std::vector<Cartographic>>> ringsResult =
          parsePolygon(value.GetArray(), "MultiPolygon", 4, true);
      if (!ringsResult.value) {
        return Result<GeoJsonObject>(std::move(ringsResult.errors));
      }

      polygons.emplace_back(std::move(*ringsResult.value));
    }

    return Result<GeoJsonObject>(
        GeoJsonMultiPolygon{std::move(polygons), boundingBox, foreignMembers},
        errorList);
  }

  return Result<GeoJsonObject>(
      ErrorList::error(fmt::format("Unknown GeoJSON object type: '{}'", type)));
}
} // namespace

Result<GeoJsonObject>
GeoJsonDocument::parseGeoJson(const rapidjson::Document& doc) {
  if (!doc.IsObject()) {
    return Result<GeoJsonObject>(
        ErrorList::error("GeoJSON must contain a JSON object."));
  }

  return parseGeoJsonObject(
      doc.GetObject(),
      [](const std::string&) { return true; },
      "");
}

Result<GeoJsonObject>
GeoJsonDocument::parseGeoJson(const std::span<const std::byte>& bytes) {
  rapidjson::Document d;
  d.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (d.HasParseError()) {
    return Result<GeoJsonObject>(ErrorList::error(fmt::format(
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
  Result<GeoJsonObject> parseResult = document.parseGeoJson(bytes);
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
  Result<GeoJsonObject> parseResult = geoJsonDocument.parseGeoJson(document);
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
    GeoJsonObject&& rootObject,
    std::vector<VectorDocumentAttribution>&& attributions)
    : _attributions(std::move(attributions)),
      _rootObject(std::move(rootObject)) {}
} // namespace CesiumVectorData