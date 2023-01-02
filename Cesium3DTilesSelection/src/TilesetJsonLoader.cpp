#include "TilesetJsonLoader.h"

#include "ImplicitOctreeLoader.h"
#include "ImplicitQuadtreeLoader.h"
#include "logTileLoadResult.h"

#include <Cesium3DTilesSelection/GltfConverters.h>
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

#include <cctype>

namespace Cesium3DTilesSelection {
namespace {
struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured
  // container. Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult<TilesetJsonLoader>>
      pExternalTilesetLoaders;
  TilesetJsonLoader* tilesetJsonLoader;

  void operator()(Tile& tile) {
    TileContent& content = tile.getContent();
    if (content.isExternalContent()) {
      std::unique_ptr<Tile>& pExternalRoot = pExternalTilesetLoaders->pRootTile;
      if (pExternalRoot) {
        // propagate all the external tiles to be the children of this tile
        std::vector<Tile> children;
        children.emplace_back(std::move(*pExternalRoot));
        tile.createChildTiles(std::move(children));

        // save the loader of the external tileset in this loader
        tilesetJsonLoader->addChildLoader(
            std::move(pExternalTilesetLoaders->pLoader));
      }
    }
  }
};

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

  // SPDLOG_LOGGER_WARN(
  //     pLogger,
  //     "The tileset contains a gltfUpAxis property. "
  //     "This property is not part of the specification. "
  //     "All glTF content should use the Y-axis as the up-axis.");

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
  std::vector<Tile> implicitRootTile;
  implicitRootTile.emplace_back(pImplicitLoader);
  implicitRootTile[0].setTransform(implicitTile.getTransform());
  implicitRootTile[0].setBoundingVolume(implicitTile.getBoundingVolume());
  implicitRootTile[0].setGeometricError(implicitTile.getGeometricError());
  implicitRootTile[0].setRefine(implicitTile.getRefine());
  implicitRootTile[0].setTileID(CesiumGeometry::QuadtreeTileID(0, 0, 0));
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
  std::vector<Tile> implicitRootTile;
  implicitRootTile.emplace_back(pImplicitLoader);
  implicitRootTile[0].setTransform(implicitTile.getTransform());
  implicitRootTile[0].setBoundingVolume(implicitTile.getBoundingVolume());
  implicitRootTile[0].setGeometricError(implicitTile.getGeometricError());
  implicitRootTile[0].setRefine(implicitTile.getRefine());
  implicitRootTile[0].setTileID(CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  implicitTile.createChildTiles(std::move(implicitRootTile));
}

void parseImplicitTileset(
    const rapidjson::Value& implicitExtensionJson,
    const char* contentUri,
    Tile& tile,
    TilesetJsonLoader& currentLoader) {
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

std::optional<Tile> parseTileJsonRecursively(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const rapidjson::Value& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    double parentGeometricError,
    TilesetJsonLoader& currentLoader) {
  if (!tileJson.IsObject()) {
    return std::nullopt;
  }

  // parse tile transform
  const std::optional<glm::dmat4x4> transform =
      CesiumUtility::JsonHelpers::getTransformProperty(tileJson, "transform");
  glm::dmat4x4 tileTransform =
      parentTransform * transform.value_or(glm::dmat4x4(1.0));

  // parse bounding volume
  std::optional<BoundingVolume> boundingVolume =
      getBoundingVolumeProperty(tileJson, "boundingVolume");
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return std::nullopt;
  }

  auto tileBoundingVolume =
      transformBoundingVolume(tileTransform, boundingVolume.value());

  // parse viewer request volume
  std::optional<BoundingVolume> tileViewerRequestVolume =
      getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
  if (tileViewerRequestVolume) {
    tileViewerRequestVolume =
        transformBoundingVolume(tileTransform, tileViewerRequestVolume.value());
  }

  // parse geometric error
  std::optional<double> geometricError =
      CesiumUtility::JsonHelpers::getScalarProperty(tileJson, "geometricError");
  if (!geometricError) {
    geometricError = parentGeometricError * 0.5;
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Tile did not contain a geometricError. "
        "Using half of the parent tile's geometric error.");
  }

  const glm::dvec3 scale = glm::dvec3(
      glm::length(tileTransform[0]),
      glm::length(tileTransform[1]),
      glm::length(tileTransform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  double tileGeometricError = geometricError.value() * maxScaleComponent;

  // parse refinement
  TileRefine tileRefine = parentRefine;
  const auto refineIt = tileJson.FindMember("refine");
  if (refineIt != tileJson.MemberEnd() && refineIt->value.IsString()) {
    std::string refine = refineIt->value.GetString();
    if (refine == "REPLACE") {
      tileRefine = TileRefine::Replace;
    } else if (refine == "ADD") {
      tileRefine = TileRefine::Add;
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
        tileRefine =
            refineUpper == "REPLACE" ? TileRefine::Replace : TileRefine::Add;
      } else {
        SPDLOG_LOGGER_WARN(
            pLogger,
            "Tile contained an unknown refine value: {}",
            refine);
      }
    }
  } else {
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
      // mark this tile as external
      Tile tile{&currentLoader, TileExternalContent{}};
      tile.setTileID("");
      tile.setTransform(tileTransform);
      tile.setBoundingVolume(tileBoundingVolume);
      tile.setViewerRequestVolume(tileViewerRequestVolume);
      tile.setGeometricError(tileGeometricError);
      tile.setRefine(tileRefine);

      parseImplicitTileset(
          implicitExtensionIt->value,
          contentUri,
          tile,
          currentLoader);

      return tile;
    }
  }

  // this is a regular tile, then parse the content bounding volume
  std::optional<BoundingVolume> tileContentBoundingVolume;
  if (hasContentMember) {
    tileContentBoundingVolume =
        getBoundingVolumeProperty(contentIt->value, "boundingVolume");
    if (tileContentBoundingVolume) {
      tileContentBoundingVolume = transformBoundingVolume(
          tileTransform,
          tileContentBoundingVolume.value());
    }
  }

  // parse tile's children
  std::vector<Tile> childTiles;
  const auto childrenIt = tileJson.FindMember("children");
  if (childrenIt != tileJson.MemberEnd() && childrenIt->value.IsArray()) {
    const auto& childrenJson = childrenIt->value;
    childTiles.reserve(childrenJson.Size());
    for (rapidjson::SizeType i = 0; i < childrenJson.Size(); ++i) {
      const auto& childJson = childrenJson[i];
      auto maybeChild = parseTileJsonRecursively(
          pLogger,
          childJson,
          tileTransform,
          tileRefine,
          tileGeometricError,
          currentLoader);

      if (maybeChild) {
        childTiles.emplace_back(std::move(*maybeChild));
      }
    }
  }

  if (contentUri) {
    Tile tile{&currentLoader};
    tile.setTileID(contentUri);
    tile.setTransform(tileTransform);
    tile.setBoundingVolume(tileBoundingVolume);
    tile.setViewerRequestVolume(tileViewerRequestVolume);
    tile.setGeometricError(tileGeometricError);
    tile.setRefine(tileRefine);
    tile.setContentBoundingVolume(tileContentBoundingVolume);
    tile.createChildTiles(std::move(childTiles));

    return tile;
  } else {
    Tile tile{&currentLoader, TileEmptyContent{}};
    tile.setTileID("");
    tile.setTransform(tileTransform);
    tile.setBoundingVolume(tileBoundingVolume);
    tile.setViewerRequestVolume(tileViewerRequestVolume);
    tile.setGeometricError(tileGeometricError);
    tile.setRefine(tileRefine);
    tile.setContentBoundingVolume(tileContentBoundingVolume);
    tile.createChildTiles(std::move(childTiles));

    return tile;
  }
}

TilesetContentLoaderResult<TilesetJsonLoader> parseTilesetJson(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const rapidjson::Document& tilesetJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine) {
  std::unique_ptr<Tile> pRootTile;
  auto gltfUpAxis = obtainGltfUpAxis(tilesetJson, pLogger);
  auto pLoader = std::make_unique<TilesetJsonLoader>(baseUrl, gltfUpAxis);
  const auto rootIt = tilesetJson.FindMember("root");
  if (rootIt != tilesetJson.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    auto maybeRootTile = parseTileJsonRecursively(
        pLogger,
        rootJson,
        parentTransform,
        parentRefine,
        10000000.0,
        *pLoader);

    if (maybeRootTile) {
      pRootTile = std::make_unique<Tile>(std::move(*maybeRootTile));
    }
  }

  return {
      std::move(pLoader),
      std::move(pRootTile),
      std::vector<LoaderCreditResult>{},
      std::vector<CesiumAsync::IAssetAccessor::THeader>{},
      ErrorList{}};
}

TilesetContentLoaderResult<TilesetJsonLoader> parseTilesetJson(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const gsl::span<const std::byte>& tilesetJsonBinary,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine) {
  rapidjson::Document tilesetJson;
  tilesetJson.Parse(
      reinterpret_cast<const char*>(tilesetJsonBinary.data()),
      tilesetJsonBinary.size());
  if (tilesetJson.HasParseError()) {
    TilesetContentLoaderResult<TilesetJsonLoader> result;
    result.errors.emplaceError(fmt::format(
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tilesetJson.GetParseError(),
        tilesetJson.GetErrorOffset()));
    return result;
  }

  return parseTilesetJson(
      pLogger,
      baseUrl,
      tilesetJson,
      parentTransform,
      parentRefine);
}

TileLoadResult parseExternalTilesetInWorkerThread(
    const glm::dmat4& tileTransform,
    CesiumGeometry::Axis upAxis,
    TileRefine tileRefine,
    const std::shared_ptr<spdlog::logger>& pLogger,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    ExternalContentInitializer&& externalContentInitializer) {
  // create external tileset
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  const auto& responseData = pResponse->data();
  const auto& tileUrl = pCompletedRequest->url();

  // Save the parsed external tileset into custom data.
  // We will propagate it back to tile later in the main
  // thread
  TilesetContentLoaderResult<TilesetJsonLoader> externalTilesetLoader =
      parseTilesetJson(
          pLogger,
          tileUrl,
          responseData,
          tileTransform,
          tileRefine);

  // check and log any errors
  const auto& errors = externalTilesetLoader.errors;
  if (errors) {
    logTileLoadResult(pLogger, tileUrl, errors);

    // since the json cannot be parsed, we don't know the content of this tile
    return TileLoadResult::createFailedResult(std::move(pCompletedRequest));
  }

  externalContentInitializer.pExternalTilesetLoaders =
      std::make_shared<TilesetContentLoaderResult<TilesetJsonLoader>>(
          std::move(externalTilesetLoader));

  // mark this tile has external content
  return TileLoadResult{
      TileExternalContent{},
      upAxis,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::move(pCompletedRequest),
      std::move(externalContentInitializer),
      TileLoadResultState::Success};
}
} // namespace

TilesetJsonLoader::TilesetJsonLoader(
    const std::string& baseUrl,
    CesiumGeometry::Axis upAxis)
    : _baseUrl{baseUrl}, _upAxis{upAxis} {}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetJsonLoader>>
TilesetJsonLoader::createLoader(
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
          TilesetContentLoaderResult<TilesetJsonLoader> result;
          result.errors.emplaceError(fmt::format(
              "Did not receive a valid response for tile content {}",
              tileUrl));
          return result;
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          TilesetContentLoaderResult<TilesetJsonLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl));
          result.statusCode = statusCode;
          return result;
        }

        return parseTilesetJson(
            pLogger,
            pCompletedRequest->url(),
            pResponse->data(),
            glm::dmat4(1.0),
            TileRefine::Replace);
      });
}

TilesetContentLoaderResult<TilesetJsonLoader> TilesetJsonLoader::createLoader(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& tilesetJsonUrl,
    const rapidjson::Document& tilesetJson) {
  return parseTilesetJson(
      pLogger,
      tilesetJsonUrl,
      tilesetJson,
      glm::dmat4(1.0),
      TileRefine::Replace);
}

CesiumAsync::Future<TileLoadResult>
TilesetJsonLoader::loadTileContent(const TileLoadInput& loadInput) {
  const Tile& tile = loadInput.tile;
  // check if this tile belongs to a child loader
  auto currentLoader = tile.getLoader();
  if (currentLoader != this) {
    return currentLoader->loadTileContent(loadInput);
  }

  // this loader only handles Url ID
  const std::string* url = std::get_if<std::string>(&tile.getTileID());
  if (!url) {
    return loadInput.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult(nullptr));
  }

  const glm::dmat4& tileTransform = tile.getTransform();
  TileRefine tileRefine = tile.getRefine();

  ExternalContentInitializer externalContentInitializer{nullptr, this};

  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;
  std::string resolvedUrl =
      CesiumUtility::Uri::resolve(this->_baseUrl, *url, true);
  return pAssetAccessor->get(asyncSystem, resolvedUrl, requestHeaders)
      .thenInWorkerThread(
          [pLogger,
           contentOptions,
           tileTransform,
           tileRefine,
           upAxis = _upAxis,
           externalContentInitializer = std::move(externalContentInitializer)](
              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                  pCompletedRequest) mutable {
            auto pResponse = pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for tile content {}",
                  tileUrl);
              return TileLoadResult::createFailedResult(
                  std::move(pCompletedRequest));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl);
              return TileLoadResult::createFailedResult(
                  std::move(pCompletedRequest));
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
                  contentOptions.ktx2TranscodeTargets;
              GltfConverterResult result = converter(responseData, gltfOptions);

              // Report any errors if there are any
              logTileLoadResult(pLogger, tileUrl, result.errors);
              if (result.errors) {
                return TileLoadResult::createFailedResult(
                    std::move(pCompletedRequest));
              }

              return TileLoadResult{
                  std::move(*result.model),
                  upAxis,
                  std::nullopt,
                  std::nullopt,
                  std::nullopt,
                  std::move(pCompletedRequest),
                  {},
                  TileLoadResultState::Success};
            } else {
              // not a renderable content, then it must be external tileset
              return parseExternalTilesetInWorkerThread(
                  tileTransform,
                  upAxis,
                  tileRefine,
                  pLogger,
                  std::move(pCompletedRequest),
                  std::move(externalContentInitializer));
            }
          });
}

TileChildrenResult TilesetJsonLoader::createTileChildren(const Tile& tile) {
  auto pLoader = tile.getLoader();
  if (pLoader != this) {
    return pLoader->createTileChildren(tile);
  }

  return {{}, TileLoadResultState::Failed};
}

const std::string& TilesetJsonLoader::getBaseUrl() const noexcept {
  return this->_baseUrl;
}

CesiumGeometry::Axis TilesetJsonLoader::getUpAxis() const noexcept {
  return _upAxis;
}

void TilesetJsonLoader::addChildLoader(
    std::unique_ptr<TilesetContentLoader> pLoader) {
  this->_children.emplace_back(std::move(pLoader));
}
} // namespace Cesium3DTilesSelection
