#include "TilesetJsonLoader.h"

#include "ImplicitOctreeLoader.h"
#include "ImplicitQuadtreeLoader.h"
#include "TilesetContentLoaderResult.h"
#include "logTileLoadResult.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesReader/GroupMetadataReader.h>
#include <Cesium3DTilesReader/MetadataEntityReader.h>
#include <Cesium3DTilesReader/SchemaReader.h>
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/geometric.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumUtility;
using namespace Cesium3DTilesContent;

namespace Cesium3DTilesSelection {
namespace {
struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured
  // container. Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult<TilesetJsonLoader>>
      pExternalTilesetLoaders;
  TilesetJsonLoader* tilesetJsonLoader;
  TileExternalContent externalContent;

  void operator()(Tile& tile) {
    TileExternalContent* pExternalContent =
        tile.getContent().getExternalContent();
    if (pExternalContent) {
      *pExternalContent = std::move(externalContent);
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
    const CesiumGeospatial::Ellipsoid& ellipsoid,
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
          maximumHeight,
          ellipsoid);
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
        a[5].GetDouble(),
        ellipsoid);
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
    TilesetJsonLoader& currentLoader,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
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
      getBoundingVolumeProperty(ellipsoid, tileJson, "boundingVolume");
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return std::nullopt;
  }

  auto tileBoundingVolume =
      transformBoundingVolume(tileTransform, boundingVolume.value());

  // parse viewer request volume
  std::optional<BoundingVolume> tileViewerRequestVolume =
      getBoundingVolumeProperty(ellipsoid, tileJson, "viewerRequestVolume");
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

  // determine if tile has implicit tiling
  const rapidjson::Value* implicitTilingJson = nullptr;
  const auto implicitTilingIt = tileJson.FindMember("implicitTiling");
  if (implicitTilingIt != tileJson.MemberEnd() &&
      implicitTilingIt->value.IsObject()) {
    // this is an external tile pointing to an implicit tileset
    implicitTilingJson = &implicitTilingIt->value;
  } else {
    const auto extensionIt = tileJson.FindMember("extensions");
    if (extensionIt != tileJson.MemberEnd()) {
      // this is the legacy 3D Tiles Next implicit tiling extension
      const auto& extensions = extensionIt->value;
      const auto implicitExtensionIt =
          extensions.FindMember("3DTILES_implicit_tiling");
      if (implicitExtensionIt != extensions.MemberEnd() &&
          implicitExtensionIt->value.IsObject()) {
        implicitTilingJson = &implicitExtensionIt->value;
      }
    }
  }

  if (implicitTilingJson) {
    // mark this tile as external
    Tile tile{&currentLoader, std::make_unique<TileExternalContent>()};
    tile.setTileID("");
    tile.setTransform(tileTransform);
    tile.setBoundingVolume(tileBoundingVolume);
    tile.setViewerRequestVolume(tileViewerRequestVolume);
    tile.setGeometricError(tileGeometricError);
    tile.setRefine(tileRefine);

    parseImplicitTileset(*implicitTilingJson, contentUri, tile, currentLoader);

    return tile;
  }

  // this is a regular tile, then parse the content bounding volume
  std::optional<BoundingVolume> tileContentBoundingVolume;
  if (hasContentMember) {
    tileContentBoundingVolume = getBoundingVolumeProperty(
        ellipsoid,
        contentIt->value,
        "boundingVolume");
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
          currentLoader,
          ellipsoid);

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
    TileRefine parentRefine,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  std::unique_ptr<Tile> pRootTile;
  auto gltfUpAxis = obtainGltfUpAxis(tilesetJson, pLogger);
  auto pLoader =
      std::make_unique<TilesetJsonLoader>(baseUrl, gltfUpAxis, ellipsoid);
  const auto rootIt = tilesetJson.FindMember("root");
  if (rootIt != tilesetJson.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    auto maybeRootTile = parseTileJsonRecursively(
        pLogger,
        rootJson,
        parentTransform,
        parentRefine,
        10000000.0,
        *pLoader,
        ellipsoid);

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

void parseTilesetMetadata(
    const std::string& baseUrl,
    const rapidjson::Document& tilesetJson,
    TileExternalContent& externalContent) {
  auto schemaIt = tilesetJson.FindMember("schema");
  if (schemaIt != tilesetJson.MemberEnd()) {
    Cesium3DTilesReader::SchemaReader schemaReader;
    auto schemaResult = schemaReader.readFromJson(schemaIt->value);
    if (schemaResult.value) {
      externalContent.metadata.schema = std::move(*schemaResult.value);
    }
  }

  auto schemaUriIt = tilesetJson.FindMember("schemaUri");
  if (schemaUriIt != tilesetJson.MemberEnd() && schemaUriIt->value.IsString()) {
    externalContent.metadata.schemaUri =
        CesiumUtility::Uri::resolve(baseUrl, schemaUriIt->value.GetString());
  }

  const auto metadataIt = tilesetJson.FindMember("metadata");
  if (metadataIt != tilesetJson.MemberEnd()) {
    Cesium3DTilesReader::MetadataEntityReader metadataReader;
    auto metadataResult = metadataReader.readFromJson(metadataIt->value);
    if (metadataResult.value) {
      externalContent.metadata.metadata = std::move(*metadataResult.value);
    }
  }

  const auto groupsIt = tilesetJson.FindMember("groups");
  if (groupsIt != tilesetJson.MemberEnd()) {
    Cesium3DTilesReader::GroupMetadataReader groupMetadataReader;
    auto groupsResult = groupMetadataReader.readArrayFromJson(groupsIt->value);
    if (groupsResult.value) {
      externalContent.metadata.groups = std::move(*groupsResult.value);
    }
  }
}

TileLoadResult parseExternalTilesetInWorkerThread(
    const glm::dmat4& tileTransform,
    CesiumGeometry::Axis upAxis,
    TileRefine tileRefine,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    ExternalContentInitializer&& externalContentInitializer,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  // create external tileset
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  const auto& responseData = pResponse->data();
  const auto& tileUrl = pCompletedRequest->url();

  rapidjson::Document tilesetJson;
  tilesetJson.Parse(
      reinterpret_cast<const char*>(responseData.data()),
      responseData.size());
  if (tilesetJson.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tilesetJson.GetParseError(),
        tilesetJson.GetErrorOffset());
    return TileLoadResult::createFailedResult(
        pAssetAccessor,
        std::move(pCompletedRequest));
  }

  // Save the parsed external tileset into custom data.
  // We will propagate it back to tile later in the main
  // thread
  TilesetContentLoaderResult<TilesetJsonLoader> externalTilesetLoader =
      parseTilesetJson(
          pLogger,
          tileUrl,
          tilesetJson,
          tileTransform,
          tileRefine,
          ellipsoid);

  // Populate the root tile with metadata
  parseTilesetMetadata(
      tileUrl,
      tilesetJson,
      externalContentInitializer.externalContent);

  // check and log any errors
  const auto& errors = externalTilesetLoader.errors;
  if (errors) {
    logTileLoadResult(pLogger, tileUrl, errors);

    // since the json cannot be parsed, we don't know the content of this tile
    return TileLoadResult::createFailedResult(
        pAssetAccessor,
        std::move(pCompletedRequest));
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
      pAssetAccessor,
      std::move(pCompletedRequest),
      std::move(externalContentInitializer),
      TileLoadResultState::Success,
      ellipsoid};
}

} // namespace

TilesetJsonLoader::TilesetJsonLoader(
    const std::string& baseUrl,
    CesiumGeometry::Axis upAxis,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _baseUrl{baseUrl}, _ellipsoid{ellipsoid}, _upAxis{upAxis}, _children{} {}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetJsonLoader>>
TilesetJsonLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& tilesetJsonUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  return externals.pAssetAccessor
      ->get(externals.asyncSystem, tilesetJsonUrl, requestHeaders)
      .thenInWorkerThread([ellipsoid,
                           asyncSystem = externals.asyncSystem,
                           pAssetAccessor = externals.pAssetAccessor,
                           pLogger = externals.pLogger](
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
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          TilesetContentLoaderResult<TilesetJsonLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl));
          result.statusCode = statusCode;
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        std::span<const std::byte> data = pResponse->data();

        rapidjson::Document tilesetJson;
        tilesetJson.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());
        if (tilesetJson.HasParseError()) {
          TilesetContentLoaderResult<TilesetJsonLoader> result;
          result.errors.emplaceError(fmt::format(
              "Error when parsing tileset JSON, error code {} at byte offset "
              "{}",
              tilesetJson.GetParseError(),
              tilesetJson.GetErrorOffset()));
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        return TilesetJsonLoader::createLoader(
            asyncSystem,
            pAssetAccessor,
            pLogger,
            pCompletedRequest->url(),
            pCompletedRequest->headers(),
            tilesetJson,
            ellipsoid);
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetJsonLoader>>
TilesetJsonLoader::createLoader(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& /*pAssetAccessor*/,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& tilesetJsonUrl,
    const CesiumAsync::HttpHeaders& /*requestHeaders*/,
    const rapidjson::Document& tilesetJson,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  TilesetContentLoaderResult<TilesetJsonLoader> result = parseTilesetJson(
      pLogger,
      tilesetJsonUrl,
      tilesetJson,
      glm::dmat4(1.0),
      TileRefine::Replace,
      ellipsoid);

  // Create a root tile to represent the tileset.json itself.
  std::vector<Tile> children;
  children.emplace_back(std::move(*result.pRootTile));

  result.pRootTile = std::make_unique<Tile>(
      children[0].getLoader(),
      std::make_unique<TileExternalContent>());

  result.pRootTile->setTileID("");
  result.pRootTile->setTransform(children[0].getTransform());
  result.pRootTile->setBoundingVolume(children[0].getBoundingVolume());
  result.pRootTile->setUnconditionallyRefine();
  result.pRootTile->setRefine(children[0].getRefine());
  result.pRootTile->createChildTiles(std::move(children));

  // Populate the root tile with metadata
  TileExternalContent* pExternal =
      result.pRootTile->getContent().getExternalContent();
  CESIUM_ASSERT(pExternal);
  if (pExternal) {
    parseTilesetMetadata(tilesetJsonUrl, tilesetJson, *pExternal);
  }

  return asyncSystem.createResolvedFuture(std::move(result));
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
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  const glm::dmat4& tileTransform = tile.getTransform();
  TileRefine tileRefine = tile.getRefine();

  ExternalContentInitializer externalContentInitializer{nullptr, this, {}};

  const auto& ellipsoid = this->_ellipsoid;
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
           ellipsoid,
           upAxis = _upAxis,
           externalContentInitializer = std::move(externalContentInitializer),
           pAssetAccessor,
           asyncSystem,
           requestHeaders](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                               pCompletedRequest) mutable {
            auto pResponse = pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for tile content {}",
                  tileUrl);
              return asyncSystem.createResolvedFuture(
                  TileLoadResult::createFailedResult(
                      pAssetAccessor,
                      std::move(pCompletedRequest)));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl);
              return asyncSystem.createResolvedFuture(
                  TileLoadResult::createFailedResult(
                      pAssetAccessor,
                      std::move(pCompletedRequest)));
            }

            // find gltf converter
            const auto& responseData = pResponse->data();
            auto converter = GltfConverters::getConverterByMagic(responseData);
            if (!converter) {
              converter = GltfConverters::getConverterByFileExtension(tileUrl);
            }

            if (converter) {
              // Convert to gltf
              AssetFetcher assetFetcher{
                  asyncSystem,
                  pAssetAccessor,
                  tileUrl,
                  tileTransform,
                  requestHeaders,
                  upAxis};
              CesiumGltfReader::GltfReaderOptions gltfOptions;
              gltfOptions.ktx2TranscodeTargets =
                  contentOptions.ktx2TranscodeTargets;
              gltfOptions.applyTextureTransform =
                  contentOptions.applyTextureTransform;
              return converter(responseData, gltfOptions, assetFetcher)
                  .thenImmediately(
                      [ellipsoid,
                       pLogger,
                       upAxis,
                       tileUrl,
                       pAssetAccessor,
                       pCompletedRequest = std::move(pCompletedRequest)](
                          GltfConverterResult&& result) mutable {
                        logTileLoadResult(pLogger, tileUrl, result.errors);
                        if (result.errors) {
                          return TileLoadResult::createFailedResult(
                              pAssetAccessor,
                              std::move(pCompletedRequest));
                        }
                        return TileLoadResult{
                            std::move(*result.model),
                            upAxis,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            pAssetAccessor,
                            std::move(pCompletedRequest),
                            {},
                            TileLoadResultState::Success,
                            ellipsoid};
                      });
            } else {
              // not a renderable content, then it must be external tileset
              return asyncSystem.createResolvedFuture(
                  parseExternalTilesetInWorkerThread(
                      tileTransform,
                      upAxis,
                      tileRefine,
                      pLogger,
                      pAssetAccessor,
                      std::move(pCompletedRequest),
                      std::move(externalContentInitializer),
                      ellipsoid));
            }
          });
}

TileChildrenResult TilesetJsonLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  if (pLoader != this) {
    return pLoader->createTileChildren(tile, ellipsoid);
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
