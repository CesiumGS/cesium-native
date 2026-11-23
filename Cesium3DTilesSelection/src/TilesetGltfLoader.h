#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <rapidjson/fwd.h>

#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetGltfLoader : public TilesetContentLoader {
public:
  TilesetGltfLoader(
      const std::string& baseUrl,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

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

  void addChildLoader(std::unique_ptr<TilesetContentLoader> pLoader);

  static CesiumAsync::Future<TilesetContentLoaderResult<TilesetGltfLoader>>
  createLoader(
      const TilesetExternals& externals,
      const std::string& gltfUrl,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  static CesiumAsync::Future<TilesetContentLoaderResult<TilesetGltfLoader>>
  createLoader(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& gltfUrl,
      const CesiumAsync::HttpHeaders& requestHeaders,
      CesiumGltf::Model&& gltf,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  void setExternalSchema(CesiumGltf::Schema* schema) override;
  virtual CesiumUtility::IntrusivePointer<CesiumGltf::Schema>
  getExternalSchema() override;

protected:
  void setOwnerOfNestedLoaders(TilesetContentManager& owner) noexcept override;

private:
  std::string _baseUrl;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> _pSharedAssetSystem;
  CesiumUtility::IntrusivePointer<CesiumGltf::Schema> _pExternalSchema;

  std::vector<std::unique_ptr<TilesetContentLoader>> _children;

  /**
   * @brief The axis that is used as the "up-axis" for glTF content loaded
   * through @ref TilesetGltfLoader.
   *
   * A glTF tileset may be y-up (following glTF standard) or z-up (through a
   * coordinate system specified in @ref CesiumGltf::ExtensionExtGeospatialCrs).
   * In the former, it is sufficient to apply a one-time transform to the root
   * tile, since all its children will inherit the transform. Thus, the loader's
   * axis is hardcoded and the correct transform is applied during loader
   * creation.
   */
  static const CesiumGeometry::Axis _upAxis = CesiumGeometry::Axis::Z;
};
} // namespace Cesium3DTilesSelection
