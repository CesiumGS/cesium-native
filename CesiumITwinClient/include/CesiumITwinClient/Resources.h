#pragma once

#include <CesiumGeospatial/GlobeRectangle.h>

#include <cstdint>
#include <string>

namespace CesiumITwinClient {
/**
 * @brief The API from which an `ITwinResource` originated.
 */
enum class ResourceSource : uint8_t {
  /** @brief This resource represents Cesium Curated Content. */
  CesiumCuratedContent = 1,
  /** @brief This resource represents an iModel Mesh Export. */
  MeshExport = 2,
  /** @brief This resource represents Reality Data. */
  RealityData = 3
};

/**
 * @brief The type of resource that an `ITwinResource` represents.
 */
enum class ResourceType : uint8_t {
  /** @brief This resource is a 3D Tiles tileset. */
  Tileset = 1,
  /** @brief This resource is raster overlay imagery. */
  Imagery = 2,
  /** @brief This resource is quantized mesh terrain. */
  Terrain = 3
};

/**
 * @brief A single resource obtained from the iTwin API that can be loaded by
 * Cesium Native.
 */
struct ITwinResource {
  /**
   * @brief The ID of this resource.
   *
   * - If `source` is `CesiumCuratedContent`, this is the integer ID of the
   * asset.
   * - If `source` is `MeshExport`, this is the GUID of the mesh export.
   * - If `source` is `RealityData`, this is the GUID of the reality data.
   */
  std::string id;
  /**
   * @brief The GUID of this resource's parent, if any.
   *
   * - If `source` is `CesiumCuratedContent`, this is `std::nullopt`.
   * - If `source` is `MeshExport`, this is the GUID of the iModel.
   * - If `source` is `RealityData`, this is the GUID of the iTwin.
   */
  std::optional<std::string> parentId;
  /**
   * @brief The API from which this resource originated.
   */
  ResourceSource source;
  /** @brief The display name of this resource. */
  std::string displayName;
  /** @brief The type of this resource. */
  ResourceType resourceType;
};

} // namespace CesiumITwinClient
