#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>
#include <Cesium3DTilesSelection/Exp_ImplicitOctreeLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetJsonLoader.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured
  // container. Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult> pExternalTilesetLoaders;
  TilesetJsonLoader* tilesetJsonLoader;

  void operator()(Tile& tile) {
    TileContent* pContent = tile.getContent();
    if (pContent && pContent->isExternalContent()) {
      std::unique_ptr<Tile>& pExternalRoot = pExternalTilesetLoaders->pRootTile;
      if (pExternalRoot) {
        // propagate all the external tiles to be the children of this tile
        std::vector<Tile> children(1);
        children[0] = std::move(*pExternalRoot);
        tile.createChildTiles(std::move(children));

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
    const std::vector<std::string>& errors) {
  if (!errors.empty()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load {}:\n- {}",
        url,
        CesiumUtility::joinToString(errors, "\n- "));
  }
}

void logWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const std::vector<std::string>& warnings) {
  if (!warnings.empty()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Warning when loading {}:\n- {}",
        url,
        CesiumUtility::joinToString(warnings, "\n- "));
  }
}

void logErrorsAndWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists) {
  logErrors(pLogger, url, errorLists.errors);
  logWarnings(pLogger, url, errorLists.warnings);
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

void createImplicitQuadtreeLoader(
    const char* contentUriTemplate,
    const char* subtreeUriTemplate,
    uint32_t subtreeLevels,
    uint32_t availableLevels,
    Tile& implicitTile,
    TilesetJsonLoader& currentLoader) {
  // Quadtree does not support bounding sphere subdivision
  const BoundingVolume& boundingVolume = implicitTile.getBoundingVolume();
  if (std::holds_alternative<CesiumGeometry::BoundingSphere>(boundingVolume)) {
    return;
  }

  const CesiumGeospatial::BoundingRegion* pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
  const CesiumGeometry::OrientedBoundingBox* pBox =
      std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
  const CesiumGeospatial::S2CellBoundingVolume* pS2Cell =
      std::get_if<CesiumGeospatial::S2CellBoundingVolume>(&boundingVolume);

  // the implicit loader will be the child loader of this tileset json loader
  TilesetContentLoader* pImplicitLoader = nullptr;
  if (pRegion) {
    auto pLoader = std::make_unique<ImplicitQuadtreeLoader>(
        currentLoader.getBaseUrl(),
        contentUriTemplate,
        subtreeUriTemplate,
        subtreeLevels,
        availableLevels,
        *pRegion);
    pImplicitLoader = pLoader.get();
    currentLoader.addChildLoader(std::move(pLoader));
  } else if (pBox) {
    auto pLoader = std::make_unique<ImplicitQuadtreeLoader>(
        currentLoader.getBaseUrl(),
        contentUriTemplate,
        subtreeUriTemplate,
        subtreeLevels,
        availableLevels,
        *pBox);
    pImplicitLoader = pLoader.get();
    currentLoader.addChildLoader(std::move(pLoader));
  } else if (pS2Cell) {
    auto pLoader = std::make_unique<ImplicitQuadtreeLoader>(
        currentLoader.getBaseUrl(),
        contentUriTemplate,
        subtreeUriTemplate,
        subtreeLevels,
        availableLevels,
        *pS2Cell);
    pImplicitLoader = pLoader.get();
    currentLoader.addChildLoader(std::move(pLoader));
  }

  // create an implicit root to associate with the above implicit loader
  std::vector<Tile> implicitRootTile(1);
  implicitRootTile[0].setTransform(implicitTile.getTransform());
  implicitRootTile[0].setBoundingVolume(implicitTile.getBoundingVolume());
  implicitRootTile[0].setGeometricError(implicitTile.getGeometricError());
  implicitRootTile[0].setRefine(implicitTile.getRefine());
  implicitRootTile[0].setTileID(CesiumGeometry::QuadtreeTileID(0, 0, 0));
  implicitRootTile[0].setContent(
      std::make_unique<TileContent>(pImplicitLoader));
  implicitTile.createChildTiles(std::move(implicitRootTile));
}

void createImplicitOctreeLoader(
    const char* contentUriTemplate,
    const char* subtreeUriTemplate,
    uint32_t subtreeLevels,
    uint32_t availableLevels,
    Tile& implicitTile,
    TilesetJsonLoader& currentLoader) {
  const BoundingVolume& boundingVolume = implicitTile.getBoundingVolume();
  if (std::holds_alternative<CesiumGeometry::BoundingSphere>(boundingVolume)) {
    return;
  }

  if (std::holds_alternative<CesiumGeospatial::S2CellBoundingVolume>(
          boundingVolume)) {
    return;
  }

  const CesiumGeospatial::BoundingRegion* pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
  const CesiumGeometry::OrientedBoundingBox* pBox =
      std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);

  // the implicit loader will be the child loader of this tileset json loader
  TilesetContentLoader* pImplicitLoader = nullptr;
  if (pRegion) {
    auto pLoader = std::make_unique<ImplicitOctreeLoader>(
        currentLoader.getBaseUrl(),
        contentUriTemplate,
        subtreeUriTemplate,
        subtreeLevels,
        availableLevels,
        *pRegion);
    pImplicitLoader = pLoader.get();
    currentLoader.addChildLoader(std::move(pLoader));
  } else if (pBox) {
    auto pLoader = std::make_unique<ImplicitOctreeLoader>(
        currentLoader.getBaseUrl(),
        contentUriTemplate,
        subtreeUriTemplate,
        subtreeLevels,
        availableLevels,
        *pBox);
    pImplicitLoader = pLoader.get();
    currentLoader.addChildLoader(std::move(pLoader));
  }

  // create an implicit root to associate with the above implicit loader
  std::vector<Tile> implicitRootTile(1);
  implicitRootTile[0].setTransform(implicitTile.getTransform());
  implicitRootTile[0].setBoundingVolume(implicitTile.getBoundingVolume());
  implicitRootTile[0].setGeometricError(implicitTile.getGeometricError());
  implicitRootTile[0].setRefine(implicitTile.getRefine());
  implicitRootTile[0].setTileID(CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  implicitRootTile[0].setContent(
      std::make_unique<TileContent>(pImplicitLoader));
  implicitTile.createChildTiles(std::move(implicitRootTile));
}

void parseImplicitTileset(
    const rapidjson::Value& implicitExtensionJson,
    const char* contentUri,
    Tile& tile,
    TilesetJsonLoader& currentLoader) {
  // mark this implicit tile as external tileset
  tile.setTileID("");
  tile.setContent(
      std::make_unique<TileContent>(&currentLoader, TileExternalContent{}));

  auto implicitTiling = implicitExtensionJson.GetObject();
  auto tilingSchemeIt = implicitTiling.FindMember("subdivisionScheme");
  auto subtreeLevelsIt = implicitTiling.FindMember("subtreeLevels");
  auto subtreesIt = implicitTiling.FindMember("subtrees");
  auto availableLevelsIt = implicitTiling.FindMember("availableLevels");
  if (availableLevelsIt == implicitTiling.MemberEnd()) {
    // old version of implicit uses maximumLevel instead of availableLevels.
    // They have the same semantic
    availableLevelsIt = implicitTiling.FindMember("maximumLevel");
  }

  // check that all the required properties above are available
  bool hasTilingSchemeProp = tilingSchemeIt != implicitTiling.MemberEnd() &&
                             tilingSchemeIt->value.IsString();
  bool hasSubtreeLevelsProp = subtreeLevelsIt != implicitTiling.MemberEnd() &&
                              subtreeLevelsIt->value.IsUint();
  bool hasAvailableLevelsProp =
      availableLevelsIt != implicitTiling.MemberEnd() &&
      availableLevelsIt->value.IsUint();
  bool hasSubtreesProp =
      subtreesIt != implicitTiling.MemberEnd() && subtreesIt->value.IsObject();
  if (!hasTilingSchemeProp || !hasSubtreeLevelsProp ||
      !hasAvailableLevelsProp || !hasSubtreesProp) {
    return;
  }

  auto subtrees = subtreesIt->value.GetObject();
  auto subtreesUriIt = subtrees.FindMember("uri");
  if (subtreesUriIt == subtrees.MemberEnd() ||
      !subtreesUriIt->value.IsString()) {
    return;
  }

  // create implicit loaders
  uint32_t subtreeLevels = subtreeLevelsIt->value.GetUint();
  uint32_t availableLevels = availableLevelsIt->value.GetUint();
  const char* subtreesUri = subtreesUriIt->value.GetString();
  const char* subdivisionScheme = tilingSchemeIt->value.GetString();

  if (std::strcmp(subdivisionScheme, "QUADTREE") == 0) {
    createImplicitQuadtreeLoader(
        contentUri,
        subtreesUri,
        subtreeLevels,
        availableLevels,
        tile,
        currentLoader);
  } else if (std::strcmp(subdivisionScheme, "OCTREE") == 0) {
    createImplicitOctreeLoader(
        contentUri,
        subtreesUri,
        subtreeLevels,
        availableLevels,
        tile,
        currentLoader);
  }
}

void parseTileJsonRecursively(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const rapidjson::Value& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    Tile& tile,
    TilesetJsonLoader& currentLoader) {
  if (!tileJson.IsObject()) {
    return;
  }

  // parse tile transform
  const std::optional<glm::dmat4x4> tileTransform =
      CesiumUtility::JsonHelpers::getTransformProperty(tileJson, "transform");
  glm::dmat4x4 transform =
      parentTransform * tileTransform.value_or(glm::dmat4x4(1.0));
  tile.setTransform(transform);

  // parse bounding volume
  std::optional<BoundingVolume> boundingVolume =
      getBoundingVolumeProperty(tileJson, "boundingVolume");
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return;
  }

  tile.setBoundingVolume(
      transformBoundingVolume(transform, boundingVolume.value()));

  // parse viewer request volume
  std::optional<BoundingVolume> viewerRequestVolume =
      getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
  if (viewerRequestVolume) {
    tile.setViewerRequestVolume(
        transformBoundingVolume(transform, viewerRequestVolume.value()));
  }

  // parse geometric error
  std::optional<double> geometricError =
      CesiumUtility::JsonHelpers::getScalarProperty(tileJson, "geometricError");
  if (!geometricError) {
    geometricError = tile.getNonZeroGeometricError();
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Tile did not contain a geometricError. "
        "Using half of the parent tile's geometric error.");
  }

  const glm::dvec3 scale = glm::dvec3(
      glm::length(transform[0]),
      glm::length(transform[1]),
      glm::length(transform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  tile.setGeometricError(geometricError.value() * maxScaleComponent);

  // parse refinement
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

  // Parse content member to determine tile content Url.
  const auto contentIt = tileJson.FindMember("content");
  bool hasContentMember =
      (contentIt != tileJson.MemberEnd()) && (contentIt->value.IsObject());
  const char* contentUri = nullptr;
  if (hasContentMember) {
    auto uriIt = contentIt->value.FindMember("uri");
    if (uriIt == contentIt->value.MemberEnd() || !uriIt->value.IsString()) {
      uriIt = contentIt->value.FindMember("url");
    }

    if (uriIt != contentIt->value.MemberEnd() && uriIt->value.IsString()) {
      contentUri = uriIt->value.GetString();
    }
  }

  // determine if tile points to implicit tiling extension
  const auto extensionIt = tileJson.FindMember("extensions");
  bool hasImplicitContent = false;
  if (extensionIt != tileJson.MemberEnd()) {
    // this is an external tile pointing to an implicit tileset
    const auto& extensions = extensionIt->value;
    const auto implicitExtensionIt =
        extensions.FindMember("3DTILES_implicit_tiling");
    hasImplicitContent = implicitExtensionIt != extensions.MemberEnd() &&
                         implicitExtensionIt->value.IsObject();
    if (hasImplicitContent) {
      parseImplicitTileset(
          implicitExtensionIt->value,
          contentUri,
          tile,
          currentLoader);
    }
  }

  // this is a regular tile
  if (!hasImplicitContent) {
    if (hasContentMember) {
      if (contentUri) {
        tile.setContent(std::make_unique<TileContent>(&currentLoader));
        tile.setTileID(contentUri);
      } else {
        tile.setContent(
            std::make_unique<TileContent>(&currentLoader, TileEmptyContent{}));
        tile.setTileID("");
      }

      std::optional<BoundingVolume> contentBoundingVolume =
          getBoundingVolumeProperty(contentIt->value, "boundingVolume");
      if (contentBoundingVolume) {
        tile.setContentBoundingVolume(
            transformBoundingVolume(transform, contentBoundingVolume.value()));
      }
    }

    const auto childrenIt = tileJson.FindMember("children");
    if (childrenIt != tileJson.MemberEnd() && childrenIt->value.IsArray()) {
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
            child,
            currentLoader);
      }

      tile.createChildTiles(std::move(childTiles));
    }
  }
}

TilesetContentLoaderResult parseTilesetJson(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const gsl::span<const std::byte>& tilesetJsonBinary,
    const glm::dmat4& parentTransform) {
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

  auto pRootTile = std::make_unique<Tile>();
  auto gltfUpAxis = obtainGltfUpAxis(tilesetJson, pLogger);
  auto pLoader = std::make_unique<TilesetJsonLoader>(baseUrl);
  const auto rootIt = tilesetJson.FindMember("root");
  if (rootIt != tilesetJson.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    parseTileJsonRecursively(
        pLogger,
        rootJson,
        parentTransform,
        TileRefine::Replace,
        *pRootTile,
        *pLoader);
  }

  return TilesetContentLoaderResult{
      std::move(pLoader),
      std::move(pRootTile),
      gltfUpAxis,
      std::vector<LoaderCreditResult>{},
      std::vector<CesiumAsync::IAssetAccessor::THeader>{},
      ErrorList{}};
}

TileLoadResult parseExternalTilesetInWorkerThread(
    const TileContentLoadInfo& loadInfo,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    ExternalContentInitializer&& externalContentInitializer) {
  // create external tileset
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  const auto& responseData = pResponse->data();
  const auto& tileUrl = pCompletedRequest->url();

  // Save the parsed external tileset into custom data.
  // We will propagate it back to tile later in the main
  // thread
  TilesetContentLoaderResult externalTilesetLoader = parseTilesetJson(
      loadInfo.pLogger,
      tileUrl,
      responseData,
      loadInfo.tileTransform);

  // check and log any errors
  const auto& errors = externalTilesetLoader.errors;
  if (errors) {
    logErrors(loadInfo.pLogger, tileUrl, errors.errors);
    return TileLoadResult{
        TileExternalContent{},
        TileLoadResultState::Failed,
        std::move(pCompletedRequest),
        {}};
  }

  externalContentInitializer.pExternalTilesetLoaders =
      std::make_shared<TilesetContentLoaderResult>(
          std::move(externalTilesetLoader));

  // mark this tile has external content
  return TileLoadResult{
      TileExternalContent{},
      TileLoadResultState::Success,
      std::move(pCompletedRequest),
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
            pResponse->data(),
            glm::dmat4(1.0));
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
    return asyncSystem.createResolvedFuture<TileLoadResult>(TileLoadResult{
        TileUnknownContent{},
        TileLoadResultState::Failed,
        0,
        {}});
  }

  ExternalContentInitializer externalContentInitializer{nullptr, this};

  const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor =
      loadInfo.pAssetAccessor;

  std::string resolvedUrl = CesiumUtility::Uri::resolve(_baseUrl, *url, true);
  return pAssetAccessor->get(asyncSystem, resolvedUrl, requestHeaders)
      .thenInWorkerThread(
          [loadInfo,
           externalContentInitializer = std::move(externalContentInitializer)](
              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                  pCompletedRequest) mutable {
            auto pResponse = pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  loadInfo.pLogger,
                  "Did not receive a valid response for tile content {}",
                  tileUrl);
              return TileLoadResult{
                  TileUnknownContent{},
                  TileLoadResultState::Failed,
                  std::move(pCompletedRequest),
                  {}};
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  loadInfo.pLogger,
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl);
              return TileLoadResult{
                  TileUnknownContent{},
                  TileLoadResultState::Failed,
                  std::move(pCompletedRequest),
                  {}};
            }

            // find gltf converter
            const auto& responseData = pResponse->data();
            auto converter = GltfConverters::getConverterByMagic(responseData);
            if (!converter) {
              converter = GltfConverters::getConverterByFileExtension(tileUrl);
            }

            if (converter) {
              // Convert to gltf
              CesiumGltfReader::GltfReaderOptions gltfOptions;
              gltfOptions.ktx2TranscodeTargets =
                  loadInfo.contentOptions.ktx2TranscodeTargets;
              GltfConverterResult result = converter(responseData, gltfOptions);

              // Report any errors if there are any
              logErrorsAndWarnings(loadInfo.pLogger, tileUrl, result.errors);
              if (result.errors) {
                return TileLoadResult{
                    TileRenderContent{std::nullopt},
                    TileLoadResultState::Failed,
                    std::move(pCompletedRequest),
                    {}};
              }

              return TileLoadResult{
                  TileRenderContent{std::move(result.model)},
                  TileLoadResultState::Success,
                  std::move(pCompletedRequest),
                  {}};
            } else {
              // not a renderable content, then it must be external tileset
              return parseExternalTilesetInWorkerThread(
                  loadInfo,
                  std::move(pCompletedRequest),
                  std::move(externalContentInitializer));
            }
          });
}

const std::string& TilesetJsonLoader::getBaseUrl() const noexcept {
  return _baseUrl;
}

void TilesetJsonLoader::addChildLoader(
    std::unique_ptr<TilesetContentLoader> pLoader) {
  _children.emplace_back(std::move(pLoader));
}
} // namespace Cesium3DTilesSelection
