#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>

namespace Cesium3DTilesSelection {

/**
 * @brief A factory to create a tileset loader.
 *
 * This class can be derived to improve the ease of constructing a @ref Tileset
 * from a custom @ref TilesetContentLoader.
 */
class TilesetLoaderFactory {
public:
  virtual ~TilesetLoaderFactory() = default;

  /**
   * @brief The type of a callback called when the Authorization header used by
   * a tileset loader has changed.
   */
  using AuthorizationHeaderChangeListener = std::function<
      void(const std::string& header, const std::string& headerValue)>;

  /**
   * @brief Creates an instance of the loader corresponding to this factory.
   *
   * @param externals The @ref TilesetExternals.
   * @param tilesetOptions The @ref TilesetOptions.
   * @param headerChangeListener A callback that will be called when the
   * Authorization header used by the tileset loader has changed.
   *
   * @returns A future that resolves to a @ref TilesetContentLoaderResult.
   */
  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener) const = 0;

  /**
   * @brief Returns true if a valid @ref TilesetContentLoader can be constructed
   * from this factory.
   */
  virtual bool isValid() const = 0;
};

/**
 * @brief A factory for creating a @ref TilesetContentLoader for assets from <a
 * href="https://ion.cesium.com/">Cesium ion</a>.
 */
class CesiumIonTilesetLoaderFactory : public TilesetLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading a Cesium ion asset.
   *
   * @param ionAssetID The Cesium ion asset ID to load.
   * @param ionAccessToken The Cesium ion token to use to authorize requests to
   * this asset.
   * @param ionAssetEndpointUrl The Cesium ion endpoint to use. This can be
   * changed to point to an instance of <a
   * href="https://cesium.com/platform/cesium-ion/cesium-ion-self-hosted/">Cesium
   * ion Self-Hosted</a>.
   */
  CesiumIonTilesetLoaderFactory(
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/")
      : _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken),
        _ionAssetEndpointUrl(ionAssetEndpointUrl) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener)
      const override;

  virtual bool isValid() const override { return this->_ionAssetID > 0; }

private:
  uint32_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
};

/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/cesium-curated-content/overview/">iTwin
 * Cesium Curated Content</a> API.
 */
class ITwinCesiumCuratedContentLoaderFactory : public TilesetLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading content from iTwin Cesium Curated
   * Content.
   *
   * @param iTwinCesiumContentID The ID of the item to load.
   * @param iTwinAccessToken The access token to use to access the API.
   */
  ITwinCesiumCuratedContentLoaderFactory(
      uint32_t iTwinCesiumContentID,
      const std::string& iTwinAccessToken)
      : _iTwinCesiumContentID(iTwinCesiumContentID),
        _iTwinAccessToken(iTwinAccessToken) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener)
      const override;

  virtual bool isValid() const override {
    return this->_iTwinCesiumContentID > 0;
  }

private:
  uint32_t _iTwinCesiumContentID;
  std::string _iTwinAccessToken;
};

/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/mesh-export/overview/">iModel
 * Mesh Export</a> API.
 */
class IModelMeshExportContentLoaderFactory : public TilesetLoaderFactory {
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
      const std::string& iTwinAccessToken)
      : _iModelId(iModelId),
        _exportId(exportId),
        _iTwinAccessToken(iTwinAccessToken) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener)
      const override;

  virtual bool isValid() const override {
    return !this->_iModelId.empty() && !this->_iTwinAccessToken.empty();
  }

private:
  std::string _iModelId;
  std::optional<std::string> _exportId;
  std::string _iTwinAccessToken;
};

/**
 * @brief A factory for creating a @ref TilesetContentLoader from data from the
 * <a
 * href="https://developer.bentley.com/apis/reality-management/overview/">iTwin
 * Reality Management</a> API.
 */
class ITwinRealityDataContentLoaderFactory : public TilesetLoaderFactory {
public:
  /**
   * @brief Creates a new factory for loading iTwin reality data.
   *
   * @param realityDataId The ID of the reality data to load.
   * @param iTwinId @parblock
   * The ID of the iTwin this reality data is associated with.
   *
   * As described in the <a
   * href="https://developer.bentley.com/apis/reality-management/operations/get-read-access-to-reality-data-container/">iTwin
   * Reality Management API documentation</a>:
   *
   * > The `iTwinId` parameter is optional, but it is preferable to provide it,
   * > because the permissions used to access the container are determined from
   * > the iTwin. With no iTwin specified, the operation can succeed in some
   * > cases (e.g. the user is the reality data's owner), but we recommend to
   * > provide a `iTwinId`.
   *
   * @endparblock
   * @param iTwinAccessToken The access token to use to access the API.
   */
  ITwinRealityDataContentLoaderFactory(
      const std::string& realityDataId,
      const std::optional<std::string>& iTwinId,
      const std::string& iTwinAccessToken)
      : _realityDataId(realityDataId),
        _iTwinId(iTwinId),
        _iTwinAccessToken(iTwinAccessToken) {}

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TilesetContentLoaderResult<
          Cesium3DTilesSelection::TilesetContentLoader>>
  createLoader(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const AuthorizationHeaderChangeListener& headerChangeListener)
      const override;

  virtual bool isValid() const override {
    return !this->_realityDataId.empty() && !this->_iTwinAccessToken.empty();
  }

private:
  std::string _realityDataId;
  std::optional<std::string> _iTwinId;
  std::string _iTwinAccessToken;
};
} // namespace Cesium3DTilesSelection