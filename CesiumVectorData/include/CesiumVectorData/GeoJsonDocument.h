#pragma once

#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/Library.h>

#include <memory>
#include <span>

namespace CesiumVectorData {

/**
 * @brief Attribution that must be included with a vector document.
 */
struct VectorDocumentAttribution {
  /** @brief The HTML string containing attribution information to show. */
  std::string html;
  /**
   * @brief If true, the attribution must be shown on screen. If false, it can
   * be included in a popover instead.
   */
  bool showOnScreen;
};

/**
 * @brief A vector document parsed from GeoJSON.
 *
 * The document is represented as a hierarchy of \ref GeoJsonObject values
 * starting with the root object.
 */
class CESIUMVECTORDATA_API GeoJsonDocument
    : public CesiumUtility::ReferenceCountedThreadSafe<GeoJsonDocument> {
public:
  /**
   * @brief Attempts to parse a \ref GeoJsonDocument from the provided GeoJSON.
   *
   * @param bytes The GeoJSON data to parse.
   * @param attributions Any attributions to attach to the document.
   * @param ellipsoid The ellipsoid to use for this data.
   * @returns A \ref CesiumUtility::Result containing the parsed
   * \ref GeoJsonDocument or any errors and warnings that came up while parsing.
   */
  static CesiumUtility::Result<CesiumUtility::IntrusivePointer<GeoJsonDocument>>
  fromGeoJson(
      const std::span<const std::byte>& bytes,
      std::vector<VectorDocumentAttribution>&& attributions = {},
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Attempts to parse a \ref GeoJsonDocument from the provided JSON
   * document.
   *
   * @param document The GeoJSON JSON document.
   * @param attributions Any attributions to attach to the document.
   * @param ellipsoid The ellipsoid to use for this data.
   * @returns A \ref CesiumUtility::Result containing the parsed
   * \ref GeoJsonDocument or any errors and warnings that came up while parsing.
   */
  static CesiumUtility::Result<CesiumUtility::IntrusivePointer<GeoJsonDocument>>
  fromGeoJson(
      const rapidjson::Document& document,
      std::vector<VectorDocumentAttribution>&& attributions = {},
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Attempts to load a \ref GeoJsonDocument from a Cesium ion asset.
   *
   * Currently only the GeoJSON format is supported.
   *
   * @param asyncSystem The \ref CesiumAsync::AsyncSystem.
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor.
   * @param ionAssetID The ID of the Cesium ion asset to load. This asset must
   * be a supported vector format.
   * @param ionAccessToken The access token that has access to the given asset.
   * @param ionAssetEndpointUrl The base URL of the ion REST API server, if
   * different from `https://api.cesium.com/`.
   * @param ellipsoid The ellipsoid to use for this data.
   * @returns A future that resolves into a \ref CesiumUtility::Result
   * containing the parsed \ref GeoJsonDocument or any errors and warnings that
   * came up while loading or parsing the data.
   */
  static CesiumAsync::Future<
      CesiumUtility::Result<CesiumUtility::IntrusivePointer<GeoJsonDocument>>>
  fromCesiumIonAsset(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/",
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Creates a new \ref GeoJsonDocument directly from a \ref
   * GeoJsonObject.
   */
  GeoJsonDocument(
      GeoJsonObject&& rootObject,
      std::vector<VectorDocumentAttribution>&& attributions);

  GeoJsonDocument() = default;

  /**
   * @brief Obtains a const reference to the root object of the parsed GeoJSON.
   */
  constexpr const GeoJsonObject& getRootObject() const {
    return this->_rootObject;
  }

  /**
   * @brief Obtains a reference to the root object of the parsed GeoJSON.
   */
  constexpr GeoJsonObject& getRootObject() { return this->_rootObject; }

  /**
   * @brief Obtains attribution information for this document.
   */
  const std::vector<VectorDocumentAttribution>& getAttributions() const;

private:
  CesiumUtility::Result<GeoJsonObject> parseGeoJson(
      const rapidjson::Document& doc,
      const CesiumGeospatial::Ellipsoid& ellipsoid);
  CesiumUtility::Result<GeoJsonObject> parseGeoJson(
      const std::span<const std::byte>& bytes,
      const CesiumGeospatial::Ellipsoid& ellipsoid);

  std::vector<VectorDocumentAttribution> _attributions;
  GeoJsonObject _rootObject =
      GeoJsonObject{GeoJsonPoint{CesiumGeospatial::Cartographic(0, 0, 0)}};
};
} // namespace CesiumVectorData