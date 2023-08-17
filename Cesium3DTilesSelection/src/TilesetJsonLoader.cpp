#include "TilesetJsonLoader.h"

#include "ImplicitOctreeLoader.h"
#include "ImplicitQuadtreeLoader.h"
#include "logTileLoadResult.h"

#include <Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h>
#include <Cesium3DTiles/Extension3dTilesImplicitTiling.h>
#include <Cesium3DTilesReader/TilesetReader.h>
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
    const Cesium3DTiles::Tileset& tileset,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  // gltfUpAxis isn't officially part of the 3D Tiles spec, but it's widely
  // used. Look for it in `unknownProperties`.
  auto gltfUpAxisIt = tileset.asset.unknownProperties.find("gltfUpAxis");
  if (gltfUpAxisIt == tileset.asset.unknownProperties.end()) {
    return CesiumGeometry::Axis::Y;
  }

  const CesiumUtility::JsonValue& gltfUpAxis = gltfUpAxisIt->second;
  if (!gltfUpAxis.isString()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "The gltfUpAxis property is not a string, using default (Y)");
    return CesiumGeometry::Axis::Y;
  }

  const std::string& gltfUpAxisString = gltfUpAxis.getString();
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
    const Cesium3DTiles::BoundingVolume& boundingVolumeJson) {
  const Cesium3DTiles::Extension3dTilesBoundingVolumeS2* pS2 =
      boundingVolumeJson
          .getExtension<Cesium3DTiles::Extension3dTilesBoundingVolumeS2>();
  if (pS2) {
    return CesiumGeospatial::S2CellBoundingVolume(
        CesiumGeospatial::S2CellID::fromToken(pS2->token),
        pS2->minimumHeight,
        pS2->maximumHeight);
  }

  if (boundingVolumeJson.box.size() >= 12) {
    const std::vector<double>& a = boundingVolumeJson.box;
    return CesiumGeometry::OrientedBoundingBox(
        glm::dvec3(a[0], a[1], a[2]),
        glm::dmat3(a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]));
  }

  if (boundingVolumeJson.region.size() >= 6) {
    const std::vector<double>& a = boundingVolumeJson.region;
    return CesiumGeospatial::BoundingRegion(
        CesiumGeospatial::GlobeRectangle(a[0], a[1], a[2], a[3]),
        a[4],
        a[5]);
  }

  if (boundingVolumeJson.sphere.size() >= 4) {
    const std::vector<double>& a = boundingVolumeJson.sphere;
    return CesiumGeometry::BoundingSphere(glm::dvec3(a[0], a[1], a[2]), a[3]);
  }

  return std::nullopt;
}

void createImplicitQuadtreeLoader(
    const std::string& contentUriTemplate,
    const std::string& subtreeUriTemplate,
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
    const std::string& contentUriTemplate,
    const std::string& subtreeUriTemplate,
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
    const Cesium3DTiles::ImplicitTiling& implicitJson,
    const std::string& contentUri,
    Tile& tile,
    TilesetJsonLoader& currentLoader) {
  const std::string& subdivisionScheme = implicitJson.subdivisionScheme;
  int64_t subtreeLevels = implicitJson.subtreeLevels;
  const Cesium3DTiles::Subtrees& subtrees = implicitJson.subtrees;
  int64_t availableLevels = implicitJson.availableLevels;
  if (availableLevels == 0) {
    // The old version of implicit uses maximumLevel instead of availableLevels.
    // They have the same semantics.
    auto maximumLevelIt = implicitJson.unknownProperties.find("maximumLevel");
    if (maximumLevelIt != implicitJson.unknownProperties.end() &&
        maximumLevelIt->second.isNumber()) {
      availableLevels =
          maximumLevelIt->second.getSafeNumberOrDefault<int64_t>(0LL);
    }
  }

  const std::string& subtreesUri = subtrees.uri;
  if (subtreesUri.empty())
    return;

  // create implicit loaders
  if (subdivisionScheme == "QUADTREE") {
    createImplicitQuadtreeLoader(
        contentUri,
        subtreesUri,
        uint32_t(subtreeLevels),
        uint32_t(availableLevels),
        tile,
        currentLoader);
  } else if (subdivisionScheme == "OCTREE") {
    createImplicitOctreeLoader(
        contentUri,
        subtreesUri,
        uint32_t(subtreeLevels),
        uint32_t(availableLevels),
        tile,
        currentLoader);
  }
}

namespace {

std::optional<glm::dmat4x4>
getTransformProperty(const std::vector<double>& transform) {
  if (transform.size() != 16)
    return std::nullopt;

  const std::vector<double>& a = transform;
  return std::make_optional<glm::dmat4>(
      glm::dvec4(a[0], a[1], a[2], a[3]),
      glm::dvec4(a[4], a[5], a[6], a[7]),
      glm::dvec4(a[8], a[9], a[10], a[11]),
      glm::dvec4(a[12], a[13], a[14], a[15]));
}

} // namespace

std::optional<Tile> parseTileJsonRecursively(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const Cesium3DTiles::Tile& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    double parentGeometricError,
    TilesetJsonLoader& currentLoader) {
  // parse tile transform
  std::optional<glm::dmat4x4> transform =
      getTransformProperty(tileJson.transform);
  if (!transform) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Tile's transform is invalid and will be ignored.");
    transform = glm::dmat4x4(1.0);
  }
  glm::dmat4x4 tileTransform = parentTransform * (*transform);

  // parse bounding volume
  std::optional<BoundingVolume> boundingVolume =
      getBoundingVolumeProperty(tileJson.boundingVolume);
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return std::nullopt;
  }

  Cesium3DTilesSelection::BoundingVolume tileBoundingVolume =
      transformBoundingVolume(tileTransform, boundingVolume.value());

  // parse viewer request volume
  std::optional<BoundingVolume> tileViewerRequestVolume =
      tileJson.viewerRequestVolume.has_value()
          ? getBoundingVolumeProperty(tileJson.viewerRequestVolume.value())
          : std::nullopt;
  if (tileViewerRequestVolume) {
    tileViewerRequestVolume =
        transformBoundingVolume(tileTransform, tileViewerRequestVolume.value());
  }

  double geometricError = tileJson.geometricError;
  if (geometricError == 0.0 && !tileJson.children.empty()) {
    // It doesn't make sense to have a geometric error of zero in a non-leaf
    // tile. Replace it with half the parent's geometric error.
    geometricError = parentGeometricError * 0.5;
    SPDLOG_LOGGER_WARN(
        pLogger,
        "A non-leaf tile has a geometricError of 0.0 (or it was missing "
        "entirely). Using a value of {} instead.",
        geometricError);
  }

  const glm::dvec3 scale = glm::dvec3(
      glm::length(tileTransform[0]),
      glm::length(tileTransform[1]),
      glm::length(tileTransform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  double tileGeometricError = geometricError * maxScaleComponent;

  // parse refinement
  TileRefine tileRefine = parentRefine;
  if (tileJson.refine.has_value()) {
    const std::string& refine = tileJson.refine.value();
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
            "Tile contains an unknown refine value: {}",
            refine);
      }
    }
  }

  // Parse content member to determine tile content Url.
  const std::string* pContentUri = nullptr;
  if (tileJson.content.has_value()) {
    const Cesium3DTiles::Content& content = tileJson.content.value();
    if (!content.uri.empty()) {
      pContentUri = &content.uri;
    } else {
      // Old versions of the 3D Tiles spec used "url" instead of "uri".
      auto urlIt = content.unknownProperties.find("url");
      if (urlIt != content.unknownProperties.end() &&
          urlIt->second.isString()) {
        pContentUri = &urlIt->second.getString();
      }
    }
  }

  // determine if tile has implicit tiling
  std::optional<Cesium3DTiles::ImplicitTiling> maybeImplicit;
  if (tileJson.implicitTiling.has_value()) {
    // this is an external tile pointing to an implicit tileset
    maybeImplicit = tileJson.implicitTiling.value();
  } else {
    const Cesium3DTiles::Extension3dTilesImplicitTiling* pImplicit =
        tileJson.getExtension<Cesium3DTiles::Extension3dTilesImplicitTiling>();
    if (pImplicit) {
      // This is the legacy 3D Tiles Next implicit tiling extension.
      // Convert it to the modern version.
      maybeImplicit = Cesium3DTiles::ImplicitTiling{};
      maybeImplicit->subdivisionScheme = pImplicit->subdivisionScheme;
      maybeImplicit->subtreeLevels = pImplicit->subtreeLevels;
      maybeImplicit->availableLevels = pImplicit->availableLevels;
      maybeImplicit->subtrees = pImplicit->subtrees;
    }
  }

  if (maybeImplicit) {
    // mark this tile as external
    Tile tile{&currentLoader, TileExternalContent{}};
    tile.setTileID("");
    tile.setTransform(tileTransform);
    tile.setBoundingVolume(tileBoundingVolume);
    tile.setViewerRequestVolume(tileViewerRequestVolume);
    tile.setGeometricError(tileGeometricError);
    tile.setRefine(tileRefine);

    parseImplicitTileset(
        maybeImplicit.value(),
        pContentUri ? *pContentUri : std::string(),
        tile,
        currentLoader);

    return tile;
  }

  // If this is a regular non-implicit tile, parse the content bounding volume
  std::optional<BoundingVolume> tileContentBoundingVolume;
  if (tileJson.content.has_value() &&
      tileJson.content->boundingVolume.has_value()) {
    tileContentBoundingVolume =
        getBoundingVolumeProperty(tileJson.content->boundingVolume.value());
    if (tileContentBoundingVolume) {
      tileContentBoundingVolume = transformBoundingVolume(
          tileTransform,
          tileContentBoundingVolume.value());
    }
  }

  // parse tile's children
  std::vector<Tile> childTiles;
  if (!tileJson.children.empty()) {
    const std::vector<Cesium3DTiles::Tile>& childrenJson = tileJson.children;
    childTiles.reserve(childrenJson.size());
    for (size_t i = 0; i < childrenJson.size(); ++i) {
      const Cesium3DTiles::Tile& childJson = childrenJson[i];
      std::optional<Tile> maybeChild = parseTileJsonRecursively(
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

  if (pContentUri) {
    Tile tile{&currentLoader};
    tile.setTileID(*pContentUri);
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
    const Cesium3DTilesReader::TilesetReaderResult& readerResult,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine) {
  const Cesium3DTiles::Tileset& tileset = *readerResult.tileset;
  std::unique_ptr<Tile> pRootTile;
  auto gltfUpAxis = obtainGltfUpAxis(tileset, pLogger);
  auto pLoader = std::make_unique<TilesetJsonLoader>(baseUrl, gltfUpAxis);

  // const auto metadataIt = tilesetJson.FindMember("metadata");
  // if (metadataIt != tilesetJson.MemberEnd()) {
  //   // metadataIt->value.data_
  // }

  auto maybeRootTile = parseTileJsonRecursively(
      pLogger,
      tileset.root,
      parentTransform,
      parentRefine,
      10000000.0,
      *pLoader);

  if (maybeRootTile) {
    pRootTile = std::make_unique<Tile>(std::move(*maybeRootTile));
  }

  ErrorList errors{};
  errors.errors.insert(
      errors.errors.end(),
      readerResult.errors.begin(),
      readerResult.errors.end());
  errors.warnings.insert(
      errors.warnings.end(),
      readerResult.warnings.begin(),
      readerResult.warnings.end());

  return {
      std::move(pLoader),
      std::move(pRootTile),
      std::vector<LoaderCreditResult>{},
      std::vector<CesiumAsync::IAssetAccessor::THeader>{},
      std::move(errors)};
}

TilesetContentLoaderResult<TilesetJsonLoader> parseTilesetJson(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const gsl::span<const std::byte>& tilesetJsonBinary,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine) {
  Cesium3DTilesReader::TilesetReader reader{};
  Cesium3DTilesReader::TilesetReaderResult readerResult =
      reader.readTileset(tilesetJsonBinary);

  if (!readerResult.errors.empty() || !readerResult.tileset) {
    TilesetContentLoaderResult<TilesetJsonLoader> result;
    result.errors.errors.insert(
        result.errors.errors.end(),
        readerResult.errors.begin(),
        readerResult.errors.end());
    result.errors.warnings.insert(
        result.errors.warnings.end(),
        readerResult.warnings.begin(),
        readerResult.warnings.end());
    return result;
  }

  return parseTilesetJson(
      pLogger,
      baseUrl,
      readerResult,
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
    const gsl::span<const std::byte>& tilesetJson) {
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
