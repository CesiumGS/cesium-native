#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <rapidjson/fwd.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetJsonLoader : public TilesetContentLoader {
public:
  /**
   * @brief Constructs a loader for one tileset JSON document.
   *
   * @param baseUrl The base URL used to resolve this tileset's content URIs.
   * @param upAxis The glTF up axis declared by the tileset.
   * @param ellipsoid The ellipsoid used for cartographic bounding volumes.
   * @param schemaBaseUrl The base URL used to resolve a relative
   * `MAXAR_content_geojson` `propertiesSchemaUri`. External tilesets repeat a
   * relative schema URI that is not a full URL; resolving it against the
   * external tileset location misses the file. Pass the root tileset URL.
   * An empty string uses @p baseUrl.
   * @param pSharedAssetSystem The shared asset system that caches the
   * converted schema. An empty pointer uses
   * @ref TilesetSharedAssetSystem::getDefault.
   */
  TilesetJsonLoader(
      const std::string& baseUrl,
      CesiumGeometry::Axis upAxis,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID,
      const std::string& schemaBaseUrl = {},
      const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
          pSharedAssetSystem = {});

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) override;

  const std::string& getBaseUrl() const noexcept;

  CesiumGeometry::Axis getUpAxis() const noexcept;

  const CesiumUtility::IntrusivePointer<CesiumGltf::Schema>&
  getExternalSchema() const noexcept;

  /**
   * @brief Records the `propertiesSchemaUri` from this tileset's metadata.
   *
   * The URI is resolved against the schema base URL when the schema is loaded.
   * An empty URI means this tileset has no Maxar properties schema.
   */
  void setMaxarPropertiesSchemaUri(std::optional<std::string> uri);

  void addChildLoader(std::unique_ptr<TilesetContentLoader> pLoader);

  static CesiumAsync::Future<TilesetContentLoaderResult<TilesetJsonLoader>>
  createLoader(
      const TilesetExternals& externals,
      const std::string& tilesetJsonUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  static CesiumAsync::Future<TilesetContentLoaderResult<TilesetJsonLoader>>
  createLoader(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& tilesetJsonUrl,
      const CesiumAsync::HttpHeaders& requestHeaders,
      rapidjson::Document&& tilesetJson,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID,
      const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
          pSharedAssetSystem = {});

  CesiumUtility::IntrusivePointer<CesiumGltf::Schema>
  getExternalSchema() override;

protected:
  void setOwnerOfNestedLoaders(TilesetContentManager& owner) noexcept override;

private:
  /**
   * @brief Loads this loader's Maxar schema through the shared asset depot.
   *
   * The cache key is the schema URI resolved against the schema base URL, plus
   * the request headers in a stable order. A second loader that produces the
   * same key receives the same schema object. Failures are cached by the depot
   * and leave this loader's schema empty.
   */
  CesiumAsync::SharedFuture<CesiumUtility::ResultPointer<CesiumGltf::Schema>>
  getMaxarSchema(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const std::shared_ptr<spdlog::logger>& pLogger);

  std::string _baseUrl;
  /**
   * @brief Base URI for a relative `MAXAR_content_geojson` schema URI.
   *
   * This stays the root tileset URL for external tileset loaders. Their own
   * content base URL is @ref _baseUrl.
   */
  std::string _schemaBaseUrl;
  std::optional<std::string> _maxarPropertiesSchemaUri;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> _pSharedAssetSystem;
  CesiumUtility::IntrusivePointer<CesiumGltf::Schema> _pExternalSchema;
  std::optional<CesiumAsync::SharedFuture<
      CesiumUtility::ResultPointer<CesiumGltf::Schema>>>
      _maxarSchemaFuture;

  /**
   * @brief The axis that was declared as the "up-axis" for glTF content.
   *
   * The glTF specification mandates that the Y-axis is the "up"-axis, so the
   * default value is @ref Axis::Y. Older tilesets may contain a string
   * property in the "assets" dictionary, named "gltfUpAxis", indicating a
   * different up-axis. Although the "gltfUpAxis" property is no longer part of
   * the 3D tiles specification, it is still considered for backward
   * compatibility.
   */
  CesiumGeometry::Axis _upAxis;

  std::vector<std::unique_ptr<TilesetContentLoader>> _children;
};
} // namespace Cesium3DTilesSelection
