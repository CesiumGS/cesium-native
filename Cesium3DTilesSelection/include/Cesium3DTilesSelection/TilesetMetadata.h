#pragma once

#include <Cesium3DTiles/Asset.h>
#include <Cesium3DTiles/GroupMetadata.h>
#include <Cesium3DTiles/Properties.h>
#include <Cesium3DTiles/Schema.h>
#include <Cesium3DTiles/Statistics.h>
#include <Cesium3DTilesSelection/Library.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CesiumAsync {
class AsyncSystem;
class IAssetAccessor;
} // namespace CesiumAsync

namespace Cesium3DTilesSelection {

/**
 * @brief Holds the metadata associated with a {@link Tileset} or an external
 * tileset. This holds all of the fields of {@link Cesium3DTiles::Tileset}
 * except for the root tile.
 */
class CESIUM3DTILESSELECTION_API TilesetMetadata
    : public CesiumUtility::ExtensibleObject {
public:
  ~TilesetMetadata() noexcept;

  /**
   * @brief Metadata about the entire tileset.
   */
  Cesium3DTiles::Asset asset;

  /**
   * @brief A dictionary object of metadata about per-feature properties.
   */
  std::unordered_map<std::string, Cesium3DTiles::Properties> properties;

  /**
   * @brief An object defining the structure of metadata classes and enums. When
   * this is defined, then `schemaUri` shall be undefined.
   */
  std::optional<Cesium3DTiles::Schema> schema;

  /**
   * @brief The URI (or IRI) of the external schema file. When this is defined,
   * then `schema` shall be undefined.
   */
  std::optional<std::string> schemaUri;

  /**
   * @brief An object containing statistics about metadata entities.
   */
  std::optional<Cesium3DTiles::Statistics> statistics;

  /**
   * @brief An array of groups that tile content may belong to. Each element of
   * this array is a metadata entity that describes the group. The tile content
   * `group` property is an index into this array.
   */
  std::vector<Cesium3DTiles::GroupMetadata> groups;

  /**
   * @brief A metadata entity that is associated with this tileset.
   */
  std::optional<Cesium3DTiles::MetadataEntity> metadata;

  /**
   * @brief The error, in meters, introduced if this tileset is not rendered. At
   * runtime, the geometric error is used to compute screen space error (SSE),
   * i.e., the error measured in pixels.
   */
  std::optional<double> geometricError;

  /**
   * @brief Names of 3D Tiles extensions used somewhere in this tileset.
   */
  std::vector<std::string> extensionsUsed;

  /**
   * @brief Names of 3D Tiles extensions required to properly load this tileset.
   * Each element of this array shall also be contained in `extensionsUsed`.
   */
  std::vector<std::string> extensionsRequired;

  /**
   * @brief Asynchronously loads the {@link schema} from the {@link schemaUri}.
   * If the {@link schemaUri} does not contain a value, this method does
   * nothing and returns an already-resolved future.
   *
   * Calling this method multiple times will return the same shared future each
   * time, unless the {@link schemaUri} is changed. In that case, when this
   * method is called, the previous load is canceled and the new one begins.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The asset accessor used to request the schema from
   * the schemaUri.
   * @return A future that resolves when the schema has been loaded from the
   * schemaUri.
   */
  CesiumAsync::SharedFuture<void>& loadSchemaUri(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor);

private:
  std::optional<CesiumAsync::SharedFuture<void>> _loadingFuture;
  std::optional<std::string> _loadingSchemaUri;
  std::shared_ptr<bool> _pLoadingCanceled;
};

} // namespace Cesium3DTilesSelection
