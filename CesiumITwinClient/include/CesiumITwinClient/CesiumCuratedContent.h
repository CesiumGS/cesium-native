#pragma once

#include <cstdint>
#include <string>

namespace CesiumITwinClient {

/**
 * @brief The type of content obtained from the iTwin Cesium Curated Content
 * API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contenttype
 * for more information.
 */
enum class CesiumCuratedContentType : uint8_t {
  /** @brief The content type returned is not a known type. */
  Unknown = 0,
  /** @brief The content is a 3D Tiles tileset. */
  Cesium3DTiles = 1,
  /** @brief The content is a glTF model. */
  Gltf = 2,
  /**
   * @brief The content is imagery that can be loaded with \ref
   * CesiumRasterOverlays::ITwinCesiumCuratedContentRasterOverlay.
   */
  Imagery = 3,
  /** @brief The content is quantized mesh terrain. */
  Terrain = 4,
  /** @brief The content is in the Keyhole Markup Language (KML) format. */
  Kml = 5,
  /**
   * @brief The content is in the Cesium Language (CZML) format.
   *
   * See https://github.com/AnalyticalGraphicsInc/czml-writer/wiki/CZML-Guide
   * for more information.
   */
  Czml = 6,
  /** @brief The content is in the GeoJSON format. */
  GeoJson = 7,
};

/**
 * @brief Obtains a \ref CesiumCuratedContentType value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contenttype
 * for the list of possible values.
 */
CesiumCuratedContentType
cesiumCuratedContentTypeFromString(const std::string& str);

/**
 * @brief Describes the state of the content during the upload and tiling
 * processes.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contentstatus
 * for more information.
 */
enum class CesiumCuratedContentStatus : uint8_t {
  Unknown = 0,
  AwaitingFiles = 1,
  NotStarted = 2,
  InProgress = 3,
  Complete = 4,
  Error = 5,
  DataError = 6
};

/**
 * @brief Obtains a \ref CesiumCuratedContentStatus value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contentstatus
 * for the list of possible values.
 */
CesiumCuratedContentStatus
cesiumCuratedContentStatusFromString(const std::string& str);

/**
 * @brief A single asset obtained from the iTwin Cesium Curated Content API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#content
 * for more information.
 */
struct CesiumCuratedContentAsset {
  /** @brief The unique identifier for the content. */
  uint64_t id;
  /** @brief The type of the content. */
  CesiumCuratedContentType type;
  /** @brief The name of the content. */
  std::string name;
  /** @brief A Markdown string describing the content. */
  std::string description;
  /**
   * @brief A Markdown compatible string containing any required attribution for
   * the content.
   *
   * Clients will be required to display the attribution to end users.
   */
  std::string attribution;
  /** @brief The status of the content. */
  CesiumCuratedContentStatus status;
};
} // namespace CesiumITwinClient