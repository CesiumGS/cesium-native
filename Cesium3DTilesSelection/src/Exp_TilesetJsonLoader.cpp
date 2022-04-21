#include <Cesium3DTilesSelection/Exp_TileRenderContentLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetJsonLoader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured container.
  // Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult> pExternalTilesetLoaders;
  TilesetJsonLoader* tilesetJsonLoader;

  void operator()(Tile& tile) {
    TileContent* pContent = tile.exp_GetContent();
    if (pContent->isExternalContent()) {
      std::unique_ptr<Tile>& pExternalRoot = pExternalTilesetLoaders->pRootTile;
      if (pExternalRoot) {
        // propagate all the external tiles to be the children of this tile
        tile.createChildTiles(1);
        gsl::span<Tile> children = tile.getChildren();
        children[0] = std::move(*pExternalRoot);
        children[0].setParent(&tile);

        // save the loader of the external tileset in this loader
        tilesetJsonLoader->addChildLoader(
            std::move(pExternalTilesetLoaders->pLoader));
      }
    }
  }
};

void logErrors(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists) {
  if (errorLists) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load {}:\n- {}",
        url,
        CesiumUtility::joinToString(errorLists.errors, "\n- "));
  }
}

/**
 * @brief Obtains the up-axis that should be used for glTF content of the
 * tileset.
 *
 * If the given tileset JSON does not contain an `asset.gltfUpAxis` string
 * property, then the default value of CesiumGeometry::Axis::Y is returned.
 *
 * Otherwise, a warning is printed, saying that the `gltfUpAxis` property is
 * not strictly compliant to the 3D tiles standard, and the return value
 * will depend on the string value of this property, which may be "X", "Y", or
 * "Z", case-insensitively, causing CesiumGeometry::Axis::X,
 * CesiumGeometry::Axis::Y, or CesiumGeometry::Axis::Z to be returned,
 * respectively.
 *
 * @param tileset The tileset JSON
 * @return The up-axis to use for glTF content
 */
CesiumGeometry::Axis obtainGltfUpAxis(
    const rapidjson::Document& tileset,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  const auto assetIt = tileset.FindMember("asset");
  if (assetIt == tileset.MemberEnd()) {
    return CesiumGeometry::Axis::Y;
  }
  const rapidjson::Value& assetJson = assetIt->value;
  const auto gltfUpAxisIt = assetJson.FindMember("gltfUpAxis");
  if (gltfUpAxisIt == assetJson.MemberEnd()) {
    return CesiumGeometry::Axis::Y;
  }

  SPDLOG_LOGGER_WARN(
      pLogger,
      "The tileset contains a gltfUpAxis property. "
      "This property is not part of the specification. "
      "All glTF content should use the Y-axis as the up-axis.");

  const rapidjson::Value& gltfUpAxisJson = gltfUpAxisIt->value;
  auto gltfUpAxisString = std::string(gltfUpAxisJson.GetString());
  if (gltfUpAxisString == "X" || gltfUpAxisString == "x") {
    return CesiumGeometry::Axis::X;
  }
  if (gltfUpAxisString == "Y" || gltfUpAxisString == "y") {
    return CesiumGeometry::Axis::Y;
  }
  if (gltfUpAxisString == "Z" || gltfUpAxisString == "z") {
    return CesiumGeometry::Axis::Z;
  }

  SPDLOG_LOGGER_WARN(
      pLogger,
      "Unknown gltfUpAxis: {}, using default (Y)",
      gltfUpAxisString);
  return CesiumGeometry::Axis::Y;
}

std::optional<BoundingVolume> getBoundingVolumeProperty(
    const rapidjson::Value& tileJson,
    const std::string& key) {
  const auto bvIt = tileJson.FindMember(key.c_str());
  if (bvIt == tileJson.MemberEnd() || !bvIt->value.IsObject()) {
    return std::nullopt;
  }

  const auto extensionsIt = bvIt->value.FindMember("extensions");
  if (extensionsIt != bvIt->value.MemberEnd() &&
      extensionsIt->value.IsObject()) {
    const auto s2It =
        extensionsIt->value.FindMember("3DTILES_bounding_volume_S2");
    if (s2It != extensionsIt->value.MemberEnd() && s2It->value.IsObject()) {
      std::string token = CesiumUtility::JsonHelpers::getStringOrDefault(
          s2It->value,
          "token",
          "1");
      double minimumHeight = CesiumUtility::JsonHelpers::getDoubleOrDefault(
          s2It->value,
          "minimumHeight",
          0.0);
      double maximumHeight = CesiumUtility::JsonHelpers::getDoubleOrDefault(
          s2It->value,
          "maximumHeight",
          0.0);
      return CesiumGeospatial::S2CellBoundingVolume(
          CesiumGeospatial::S2CellID::fromToken(token),
          minimumHeight,
          maximumHeight);
    }
  }

  const auto boxIt = bvIt->value.FindMember("box");
  if (boxIt != bvIt->value.MemberEnd() && boxIt->value.IsArray() &&
      boxIt->value.Size() >= 12) {
    const auto& a = boxIt->value.GetArray();
    for (rapidjson::SizeType i = 0; i < 12; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return CesiumGeometry::OrientedBoundingBox(
        glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
        glm::dmat3(
            a[3].GetDouble(),
            a[4].GetDouble(),
            a[5].GetDouble(),
            a[6].GetDouble(),
            a[7].GetDouble(),
            a[8].GetDouble(),
            a[9].GetDouble(),
            a[10].GetDouble(),
            a[11].GetDouble()));
  }

  const auto regionIt = bvIt->value.FindMember("region");
  if (regionIt != bvIt->value.MemberEnd() && regionIt->value.IsArray() &&
      regionIt->value.Size() >= 6) {
    const auto& a = regionIt->value;
    for (rapidjson::SizeType i = 0; i < 6; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return CesiumGeospatial::BoundingRegion(
        CesiumGeospatial::GlobeRectangle(
            a[0].GetDouble(),
            a[1].GetDouble(),
            a[2].GetDouble(),
            a[3].GetDouble()),
        a[4].GetDouble(),
        a[5].GetDouble());
  }

  const auto sphereIt = bvIt->value.FindMember("sphere");
  if (sphereIt != bvIt->value.MemberEnd() && sphereIt->value.IsArray() &&
      sphereIt->value.Size() >= 4) {
    const auto& a = sphereIt->value;
    for (rapidjson::SizeType i = 0; i < 4; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return CesiumGeometry::BoundingSphere(
        glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
        a[3].GetDouble());
  }

  return std::nullopt;
}

void parseTileJsonRecursively(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const rapidjson::Value& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    Tile* parentTile,
    Tile& tile,
    TilesetContentLoader& currentLoader) {
  if (!tileJson.IsObject()) {
    return;
  }

  tile.setParent(parentTile);
  tile.exp_SetContent(std::make_unique<TileContent>(&currentLoader));

  const std::optional<glm::dmat4x4> tileTransform =
      CesiumUtility::JsonHelpers::getTransformProperty(tileJson, "transform");
  glm::dmat4x4 transform =
      parentTransform * tileTransform.value_or(glm::dmat4x4(1.0));
  tile.setTransform(transform);

  const auto contentIt = tileJson.FindMember("content");
  const auto childrenIt = tileJson.FindMember("children");

  const char* contentUri = nullptr;
  if (contentIt != tileJson.MemberEnd() && contentIt->value.IsObject()) {
    auto uriIt = contentIt->value.FindMember("uri");
    if (uriIt == contentIt->value.MemberEnd() || !uriIt->value.IsString()) {
      uriIt = contentIt->value.FindMember("url");
    }

    if (uriIt != contentIt->value.MemberEnd() && uriIt->value.IsString()) {
      contentUri = uriIt->value.GetString();
      tile.setTileID(contentUri);
    }

    std::optional<BoundingVolume> contentBoundingVolume =
        getBoundingVolumeProperty(contentIt->value, "boundingVolume");
    if (contentBoundingVolume) {
      tile.setContentBoundingVolume(
          transformBoundingVolume(transform, contentBoundingVolume.value()));
    }
  }

  std::optional<BoundingVolume> boundingVolume =
      getBoundingVolumeProperty(tileJson, "boundingVolume");
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return;
  }

  std::optional<double> geometricError =
      CesiumUtility::JsonHelpers::getScalarProperty(tileJson, "geometricError");
  if (!geometricError) {
    geometricError = tile.getNonZeroGeometricError();
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Tile did not contain a geometricError. "
        "Using half of the parent tile's geometric error.");
  }

  tile.setBoundingVolume(
      transformBoundingVolume(transform, boundingVolume.value()));
  const glm::dvec3 scale = glm::dvec3(
      glm::length(transform[0]),
      glm::length(transform[1]),
      glm::length(transform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  tile.setGeometricError(geometricError.value() * maxScaleComponent);

  std::optional<BoundingVolume> viewerRequestVolume =
      getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
  if (viewerRequestVolume) {
    tile.setViewerRequestVolume(
        transformBoundingVolume(transform, viewerRequestVolume.value()));
  }

  const auto refineIt = tileJson.FindMember("refine");
  if (refineIt != tileJson.MemberEnd() && refineIt->value.IsString()) {
    std::string refine = refineIt->value.GetString();
    if (refine == "REPLACE") {
      tile.setRefine(TileRefine::Replace);
    } else if (refine == "ADD") {
      tile.setRefine(TileRefine::Add);
    } else {
      std::string refineUpper = refine;
      std::transform(
          refineUpper.begin(),
          refineUpper.end(),
          refineUpper.begin(),
          [](unsigned char c) -> unsigned char {
            return static_cast<unsigned char>(std::toupper(c));
          });
      if (refineUpper == "REPLACE" || refineUpper == "ADD") {
        SPDLOG_LOGGER_WARN(
            pLogger,
            "Tile refine value '{}' should be uppercase: '{}'",
            refine,
            refineUpper);
        tile.setRefine(
            refineUpper == "REPLACE" ? TileRefine::Replace : TileRefine::Add);
      } else {
        SPDLOG_LOGGER_WARN(
            pLogger,
            "Tile contained an unknown refine value: {}",
            refine);
      }
    }
  } else {
    tile.setRefine(parentRefine);
  }

  // Check for the 3DTILES_implicit_tiling extension
  if (childrenIt == tileJson.MemberEnd()) {
    if (contentUri) {
      // parseImplicitTileset(tile, tileJson, contentUri, context, newContexts);
    }
  } else if (childrenIt->value.IsArray()) {
    const auto& childrenJson = childrenIt->value;
    std::vector<Tile> childTiles(childrenJson.Size());
    for (rapidjson::SizeType i = 0; i < childrenJson.Size(); ++i) {
      const auto& childJson = childrenJson[i];
      Tile& child = childTiles[i];
      parseTileJsonRecursively(
          pLogger,
          childJson,
          transform,
          tile.getRefine(),
          &tile,
          child,
          currentLoader);
    }

    tile.createChildTiles(std::move(childTiles));
  }
}

TilesetContentLoaderResult parseTilesetJson(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const gsl::span<const std::byte>& tilesetJsonBinary) {
  rapidjson::Document tilesetJson;
  tilesetJson.Parse(
      reinterpret_cast<const char*>(tilesetJsonBinary.data()),
      tilesetJsonBinary.size());
  if (tilesetJson.HasParseError()) {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tilesetJson.GetParseError(),
        tilesetJson.GetErrorOffset()));
    return result;
  }

  TilesetContentLoaderResult result;
  result.pRootTile = std::make_unique<Tile>();
  result.gltfUpAxis = obtainGltfUpAxis(tilesetJson, pLogger);
  result.pLoader = std::make_unique<TilesetJsonLoader>(baseUrl);
  if (result.errors) {
    return result;
  }

  const auto rootIt = tilesetJson.FindMember("root");
  if (rootIt != tilesetJson.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    parseTileJsonRecursively(
        pLogger,
        rootJson,
        glm::dmat4(1.0),
        TileRefine::Replace,
        nullptr,
        *result.pRootTile,
        *result.pLoader);
  }

  return result;
}

TileLoadResult parseExternalTilesetInWorkerThread(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& pCompletedRequest,
    const std::shared_ptr<spdlog::logger>& pLogger,
    ExternalContentInitializer&& externalContentInitializer) {
  // create external tileset
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  const auto& responseData = pResponse->data();
  const auto& tileUrl = pCompletedRequest->url();
  uint16_t statusCode = pResponse->statusCode();

  // Save the parsed external tileset into custom data.
  // We will propagate it back to tile later in the main
  // thread
  externalContentInitializer.pExternalTilesetLoaders =
      std::make_shared<TilesetContentLoaderResult>(
          parseTilesetJson(pLogger, tileUrl, responseData));

  // check and log any errors
  const auto& errors =
      externalContentInitializer.pExternalTilesetLoaders->errors;
  if (errors) {
    logErrors(pLogger, tileUrl, errors);
    return {TileExternalContent{}, TileLoadState::Failed, statusCode, {}};
  }

  // mark this tile has external content
  return {
      TileExternalContent{},
      TileLoadState::ContentLoaded,
      statusCode,
      std::move(externalContentInitializer)};
}
} // namespace

TilesetJsonLoader::TilesetJsonLoader(const std::string& baseUrl)
    : _baseUrl{baseUrl} {}

CesiumAsync::Future<TilesetContentLoaderResult> TilesetJsonLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& tilesetJsonUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  return externals.pAssetAccessor
      ->get(externals.asyncSystem, tilesetJsonUrl, requestHeaders)
      .thenInWorkerThread([pLogger = externals.pLogger](
                              const std::shared_ptr<CesiumAsync::IAssetRequest>&
                                  pCompletedRequest) {
        const CesiumAsync::IAssetResponse* pResponse =
            pCompletedRequest->response();
        const std::string& tileUrl = pCompletedRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult result;
          result.errors.emplace_error(fmt::format(
              "Did not receive a valid response for tile content {}",
              tileUrl));
          return result;
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          TilesetContentLoaderResult result;
          result.errors.emplace_error(fmt::format(
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl));
          return result;
        }

        return parseTilesetJson(
            pLogger,
            pCompletedRequest->url(),
            pResponse->data());
      });
}

CesiumAsync::Future<TileLoadResult> TilesetJsonLoader::loadTileContent(
    TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  // check if this tile belongs to a child loader
  if (&currentLoader != this) {
    return currentLoader.loadTileContent(
        currentLoader,
        loadInfo,
        requestHeaders);
  }

  const CesiumAsync::AsyncSystem& asyncSystem = loadInfo.asyncSystem;

  // this loader only handles Url ID
  const std::string* url = std::get_if<std::string>(&loadInfo.tileID);
  if (!url) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{TileUnknownContent{}, TileLoadState::Failed, 0, {}});
  }

  ExternalContentInitializer externalContentInitializer{nullptr, this};

  const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor =
      loadInfo.pAssetAccessor;

  return pAssetAccessor->get(asyncSystem, *url, requestHeaders)
      .thenInWorkerThread(
          [loadInfo,
           externalContentInitializer = std::move(externalContentInitializer)](
              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                  pCompletedRequest) mutable {
            return TileRenderContentLoader::load(pCompletedRequest, loadInfo)
                .thenImmediately(
                    [pCompletedRequest,
                     loadInfo,
                     externalContentInitializer =
                         std::move(externalContentInitializer)](
                        TileRenderContentLoadResult&& result) mutable
                    -> TileLoadResult {
                      auto pResponse = pCompletedRequest->response();
                      uint16_t statusCode = pResponse->statusCode();
                      if (result.reason ==
                          TileRenderContentFailReason::DataRequestFailed) {
                        return {
                            std::move(result.content),
                            TileLoadState::FailedTemporarily,
                            statusCode,
                            {}};
                      } else if (
                          result.reason ==
                          TileRenderContentFailReason::ConversionFailed) {
                        return {
                            std::move(result.content),
                            TileLoadState::Failed,
                            statusCode,
                            {}};
                      } else if (
                          result.reason ==
                          TileRenderContentFailReason::UnsupportedFormat) {
                        return parseExternalTilesetInWorkerThread(
                            pCompletedRequest,
                            loadInfo.pLogger,
                            std::move(externalContentInitializer));
                      } else {
                        return {
                            std::move(result.content),
                            TileLoadState::ContentLoaded,
                            statusCode,
                            {}};
                      }
                    });
          });
}

void TilesetJsonLoader::addChildLoader(
    std::unique_ptr<TilesetContentLoader> pLoader) {
  _children.emplace_back(std::move(pLoader));
}
} // namespace Cesium3DTilesSelection
