#include "TilesetContentLoader.h"
#include "TilesetContentLoaderFactory.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetExternals.h"

#include <CesiumAsync/Future.h>

#include <string>

namespace Cesium3DTilesSelection {
/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/mesh-export/overview/">iModel
 * Mesh Export</a> API.
 */
class IModelMeshExportContentLoaderFactory
    : public TilesetContentLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading content from an iModel Mesh
   * Export.
   *
   * @param iModelId The ID of the iModel to load a Mesh Export of.
   * @param exportId The ID of a specific mesh export to use, or `std::nullopt`
   * to use the most recently modified export.
   * @param iTwinAccessToken The access token to use to access the API.
   */
  IModelMeshExportContentLoaderFactory(
      const std::string& iModelId,
      const std::optional<std::string>& exportId,
      const std::string& iTwinAccessToken);

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) override;

  virtual bool isValid() const override;

private:
  std::string _iModelId;
  std::optional<std::string> _exportId;
  std::string _iTwinAccessToken;
};
} // namespace Cesium3DTilesSelection