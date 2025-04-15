#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorNode.h>

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
 * @brief A vector document parsed from a format like GeoJSON.
 *
 * The document is represented as a hierarchy of \ref VectorNode values starting
 * with the root node.
 */
class CESIUMVECTORDATA_API VectorDocument {
public:
  /**
   * @brief Attempts to parse a \ref VectorDocument from the provided GeoJSON.
   *
   * @param bytes The GeoJSON data to parse.
   * @param attributions Any attributions to attach to the document.
   * @returns A \ref CesiumUtility::Result containing the parsed
   * \ref VectorDocument or any errors and warnings that came up while parsing.
   */
  static CesiumUtility::Result<VectorDocument> fromGeoJson(
      const std::span<const std::byte>& bytes,
      std::vector<VectorDocumentAttribution>&& attributions = {});

  /**
   * @brief Attempts to parse a \ref VectorDocument from the provided JSON
   * document.
   *
   * @param document The GeoJSON JSON document.
   * @param attributions Any attributions to attach to the document.
   * @returns A \ref CesiumUtility::Result containing the parsed
   * \ref VectorDocument or any errors and warnings that came up while parsing.
   */
  static CesiumUtility::Result<VectorDocument> fromGeoJson(
      const rapidjson::Document& document,
      std::vector<VectorDocumentAttribution>&& attributions = {});

  /**
   * @brief Attempts to load a \ref VectorDocument from a Cesium ion asset.
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
   * @returns A future that resolves into a \ref CesiumUtility::Result
   * containing the parsed \ref VectorDocument or any errors and warnings that
   * came up while loading or parsing the data.
   */
  static CesiumAsync::Future<CesiumUtility::Result<VectorDocument>>
  fromCesiumIonAsset(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

  /**
   * @brief Creates a new \ref VectorDocument directly from a \ref VectorNode.
   */
  VectorDocument(
      VectorNode&& rootNode,
      std::vector<VectorDocumentAttribution>&& attributions);

  /**
   * @brief Obtains the root node of this \ref VectorDocument.
   */
  const VectorNode& getRootNode() const;
  /**
   * @brief Obtains the root node of this \ref VectorDocument.
   */
  VectorNode& getRootNode();

  /**
   * @brief Obtains attribution information for this document.
   */
  const std::vector<VectorDocumentAttribution>& getAttributions() const;

private:
  VectorNode _rootNode;

  std::vector<VectorDocumentAttribution> _attributions;
};
} // namespace CesiumVectorData