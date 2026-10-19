#include "TilesetGltfLoader.h"

#include "ImplicitOctreeLoader.h"
#include "ImplicitQuadtreeLoader.h"
#include "logTileLoadResult.h"

#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTiles/TileBoundingVolumes.h>
#include <Cesium3DTiles/TileRefine.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetContentOptions.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
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
#include <CesiumGltf/Extension3dTilesImplicitTiling.h>
#include <CesiumGltf/Extension3dTilesTilesetVoxels.h>
#include <CesiumGltf/ExtensionExtGeospatialCrs.h>
#include <CesiumGltf/ExtensionExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModel3dTilesTileset.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionNode3dTilesTileset.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfConverters/GltfConverterResult.h>
#include <CesiumGltfConverters/GltfConverters.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GltfConverter.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumGltfConverters;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
struct ExternalContentInitializer {
  // Have to use shared_ptr here to make this functor copyable. Otherwise,
  // std::function won't work with move-only type as it's a type-erasured
  // container. Unfortunately, std::move_only_function is scheduled for C++23.
  std::shared_ptr<TilesetContentLoaderResult<TilesetGltfLoader>>
      pExternalTilesetLoaders;
  TilesetGltfLoader* tilesetJsonLoader;
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
    const CesiumGltf::Model& model,
    const std::shared_ptr<spdlog::logger>& /*pLogger*/) {
  if (const auto* pCrsExtension =
          model.getExtension<CesiumGltf::ExtensionExtGeospatialCrs>()) {
    // TODO: Handle EXT_geospatial_crs
  }
  return CesiumGeometry::Axis::Y;
}

void createImplicitQuadtreeLoader(
    const std::string& contentUriTemplate,
    const std::string& subtreeUriTemplate,
    uint32_t subtreeLevels,
    uint32_t availableLevels,
    Tile& implicitTile,
    TilesetGltfLoader& currentLoader) {
  std::optional<ImplicitQuadtreeBoundingVolume> quadtreeBoundingVolume =
      getAsImplicitQuadtreeBoundingVolume(implicitTile.getBoundingVolume());
  if (!quadtreeBoundingVolume) {
    return;
  }

  auto pLoader = std::make_unique<ImplicitQuadtreeLoader>(
      currentLoader.getBaseUrl(),
      contentUriTemplate,
      subtreeUriTemplate,
      subtreeLevels,
      availableLevels,
      *quadtreeBoundingVolume);

  TilesetContentLoader* pImplicitLoader = pLoader.get();
  currentLoader.addChildLoader(std::move(pLoader));

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
    TilesetGltfLoader& currentLoader) {
  std::optional<ImplicitOctreeBoundingVolume> octreeBoundingVolume =
      getAsImplicitOctreeBoundingVolume(implicitTile.getBoundingVolume());
  if (!octreeBoundingVolume) {
    return;
  }

  auto pLoader = std::make_unique<ImplicitOctreeLoader>(
      currentLoader.getBaseUrl(),
      contentUriTemplate,
      subtreeUriTemplate,
      subtreeLevels,
      availableLevels,
      *octreeBoundingVolume);

  TilesetContentLoader* pImplicitLoader = pLoader.get();
  currentLoader.addChildLoader(std::move(pLoader));

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
    const CesiumGltf::Extension3dTilesImplicitTiling& implicitTilingExtension,
    Tile& tile,
    TilesetGltfLoader& currentLoader) {
  uint32_t subtreeLevels =
      static_cast<uint32_t>(implicitTilingExtension.subtreeLevels);
  uint32_t availableLevels =
      static_cast<uint32_t>(implicitTilingExtension.availableLevels);
  const std::string& contentUri = implicitTilingExtension.contentUri;
  const std::string& subtreesUri = implicitTilingExtension.subtreeUri;
  const std::string& subdivisionScheme =
      implicitTilingExtension.subdivisionScheme;

  if (subdivisionScheme ==
      Extension3dTilesImplicitTiling::SubdivisionScheme::QUADTREE) {
    createImplicitQuadtreeLoader(
        contentUri,
        subtreesUri,
        subtreeLevels,
        availableLevels,
        tile,
        currentLoader);
  } else if (
      subdivisionScheme ==
      Extension3dTilesImplicitTiling::SubdivisionScheme::OCTREE) {
    createImplicitOctreeLoader(
        contentUri,
        subtreesUri,
        subtreeLevels,
        availableLevels,
        tile,
        currentLoader);
  }
}

std::optional<Tile> parseGltfNodeRecursively(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    const glm::dmat4& parentTransform,
    Cesium3DTiles::TileRefine parentRefine,
    double /*parentGeometricError*/,
    TilesetGltfLoader& currentLoader,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const auto* pTilesetExtension =
      node.getExtension<CesiumGltf::ExtensionNode3dTilesTileset>();
  if (!pTilesetExtension) {
    // Per the spec, `3DTILES_tileset` MUST be defined on all nodes, so this is
    // an error.
    return std::nullopt;
  }

  // Parse tile transform
  const std::optional<glm::dmat4x4> transform =
      CesiumGltfContent::GltfUtilities::getNodeTransform(node);
  glm::dmat4x4 tileTransform =
      parentTransform * transform.value_or(glm::dmat4x4(1.0));

  const CesiumGltf::Shape* pBoundingVolume =
      node.boundingVolume
          ? model.getSafe(&model.shapes, node.boundingVolume->shape)
          : nullptr;

  if (!pBoundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Node did not contain a boundingVolume");
    return std::nullopt;
  }

  CesiumUtility::Result<Cesium3DTiles::BoundingVolume> boundingVolume =
      Cesium3DTiles::getBoundingVolume(*pBoundingVolume, ellipsoid);
  if (!boundingVolume.value) {
    // TODO: LOG
    return std::nullopt;
  }

  auto tileBoundingVolume = Cesium3DTiles::transformBoundingVolume(
      tileTransform,
      *boundingVolume.value);

  // parse geometric error
  double geometricError = pTilesetExtension->geometricError;

  const glm::dvec3 scale = glm::dvec3(
      glm::length(tileTransform[0]),
      glm::length(tileTransform[1]),
      glm::length(tileTransform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  double tileGeometricError = geometricError * maxScaleComponent;

  // parse refinement
  Cesium3DTiles::TileRefine tileRefine = parentRefine;
  if (pTilesetExtension->refine) {
    CesiumUtility::Result<Cesium3DTiles::TileRefine> parsedRefine =
        Cesium3DTiles::getTileRefine(*pTilesetExtension->refine);
    if (parsedRefine.value) {
      tileRefine = *parsedRefine.value;
    } else {
      for (const std::string& error : parsedRefine.errors.errors) {
        SPDLOG_LOGGER_WARN(pLogger, error);
      }
      SPDLOG_LOGGER_WARN(
          pLogger,
          "Found errors while parsing tile's refine value. The parent value "
          "will be used instead.");
    }
    for (const std::string& error : parsedRefine.errors.warnings) {
      SPDLOG_LOGGER_WARN(pLogger, error);
    }
  }

  // Parse the external asset to determine tile content URL, if present.
  const CesiumGltf::File* pFile = nullptr;
  const CesiumGltf::ExternalAsset* pExternalAsset =
      model.getSafe(&model.externalAssets, node.externalAsset);
  if (pExternalAsset) {
    pFile = model.getSafe(&model.files, pExternalAsset->file);
  }

  const char* contentUri = pFile && pFile->uri ? pFile->uri->c_str() : nullptr;
  // TODO: what about file bufferview?

  if (const auto* pImplicitTiling =
          node.getExtension<CesiumGltf::Extension3dTilesImplicitTiling>()) {
    // If the tile has implicit tiling, mark it as external.
    auto pExternalContent = std::make_unique<TileExternalContent>();

    // Check for 3DTILES_tileset_voxels.
    if (const auto* pVoxels =
            model.getExtension<CesiumGltf::Extension3dTilesTilesetVoxels>()) {
      pExternalContent->extensions.emplace(
          Extension3dTilesTilesetVoxels::ExtensionName,
          *pVoxels);
    }

    Tile tile{&currentLoader, TileID(), std::move(pExternalContent)};
    tile.setTransform(tileTransform);
    tile.setBoundingVolume(tileBoundingVolume);
    tile.setGeometricError(tileGeometricError);
    tile.setRefine(tileRefine);

    parseImplicitTileset(*pImplicitTiling, tile, currentLoader);

    return tile;
  }

  // This is a non-implicit tile, so parse the content bounding volume.
  std::optional<Cesium3DTiles::BoundingVolume> tileContentBoundingVolume;
  const CesiumGltf::Shape* pContentBoundingVolume = nullptr;

  if (pTilesetExtension->content &&
      pTilesetExtension->content->boundingVolume) {
    pContentBoundingVolume = model.getSafe(
        &model.shapes,
        pTilesetExtension->content->boundingVolume->shape);
  }

  if (pContentBoundingVolume) {
    CesiumUtility::Result<Cesium3DTiles::BoundingVolume> contentBoundingVolume =
        Cesium3DTiles::getBoundingVolume(*pContentBoundingVolume, ellipsoid);
    if (contentBoundingVolume.value) {
      tileContentBoundingVolume = Cesium3DTiles::transformBoundingVolume(
          tileTransform,
          contentBoundingVolume.value.value());
    }
  }

  // Recurse over node children.
  std::vector<Tile> childTiles;
  for (int32_t childIndex : node.children) {
    const CesiumGltf::Node* pChild = model.getSafe(&model.nodes, childIndex);
    if (!pChild) {
      continue;
    }
    auto maybeChild = parseGltfNodeRecursively(
        pLogger,
        model,
        *pChild,
        tileTransform,
        tileRefine,
        tileGeometricError,
        currentLoader,
        ellipsoid);

    if (maybeChild) {
      childTiles.emplace_back(std::move(*maybeChild));
    }
  }

  Tile tile{&currentLoader};
  tile.setTileID(contentUri ? contentUri : std::string{});
  tile.setTransform(tileTransform);
  tile.setBoundingVolume(tileBoundingVolume);
  tile.setGeometricError(tileGeometricError);
  tile.setRefine(tileRefine);
  tile.setContentBoundingVolume(tileContentBoundingVolume);
  tile.createChildTiles(std::move(childTiles));

  return tile;
}

TilesetContentLoaderResult<TilesetGltfLoader> parseTilesetGltf(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    const CesiumGltf::Model& model,
    const glm::dmat4& parentTransform,
    Cesium3DTiles::TileRefine parentRefine,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumGeometry::Axis gltfUpAxis = obtainGltfUpAxis(model, pLogger);
  auto pLoader =
      std::make_unique<TilesetGltfLoader>(baseUrl, gltfUpAxis, ellipsoid);

  int32_t sceneIndex = std::max(model.scene, 0);
  const CesiumGltf::Scene* pScene = model.getSafe(&model.scenes, sceneIndex);

  int32_t rootNodeIndex = 0;
  if (pScene && pScene->nodes.size() > 0) {
    rootNodeIndex = pScene->nodes[0];
  }

  std::unique_ptr<Tile> pRootTile;
  const CesiumGltf::Node* pRootNode =
      model.getSafe(&model.nodes, rootNodeIndex);

  if (pRootNode) {
    auto maybeRootTile = parseGltfNodeRecursively(
        pLogger,
        model,
        *pRootNode,
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
      std::move(requestHeaders),
      ErrorList{}};
}

void parseTilesetMetadata(
    const std::shared_ptr<spdlog::logger>& /*pLogger*/,
    const std::string& /*baseUrl*/,
    CesiumGltf::Model&& tilesetGltf,
    TileExternalContent& externalContent) {
  Cesium3DTilesSelection::TilesetMetadata& metadata = externalContent.metadata;

  metadata.extensions = std::move(tilesetGltf.extensions);
  metadata.extensionsRequired = std::move(tilesetGltf.extensionsRequired);
  metadata.extensionsUsed = std::move(tilesetGltf.extensionsUsed);
  metadata.extras = std::move(tilesetGltf.extras);

  // "Downgrade" to older format.
  // TODO: Rework TilesetMetadata to be more generic.
  // CesiumTilesetJson::Asset asset;
  // asset.extensions = std::move(tilesetGltf.asset.extensions);
  // asset.version = tilesetGltf.asset.version;
  //// TODO: copyright
  // metadata.asset = asset;

  // const auto* pMetadataExtension =
  //     tilesetGltf
  //         .getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  // if (pMetadataExtension) {
  // }
  // metadata.geometricError = tileset.geometricError;
  // metadata.groups = std::move(tileset.groups);
  // metadata.metadata = std::move(tileset.metadata);
  // metadata.properties = std::move(tileset.properties);
  // metadata.schema = std::move(tileset.schema);
  // if (tileset.schemaUri) {
  //   metadata.schemaUri =
  //       CesiumUtility::Uri::resolve(baseUrl, *tileset.schemaUri);
  // }
  // metadata.statistics = std::move(tileset.statistics);
  // metadata.unknownProperties = std::move(tilesetGltf.unknownProperties);
}

TileLoadResult parseExternalTilesetInWorkerThread(
    const glm::dmat4& tileTransform,
    CesiumGeometry::Axis upAxis,
    Cesium3DTiles::TileRefine tileRefine,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    ExternalContentInitializer&& externalContentInitializer,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    CesiumGltf::Model&& tilesetGltf) {
  const auto& tileUrl = pCompletedRequest->url();

  // Save the parsed external tileset into custom data.
  // We will propagate it back to tile later in the main
  // thread
  TilesetContentLoaderResult<TilesetGltfLoader> externalTilesetLoader =
      parseTilesetGltf(
          pLogger,
          tileUrl,
          {pCompletedRequest->headers().begin(),
           pCompletedRequest->headers().end()},
          tilesetGltf,
          tileTransform,
          tileRefine,
          ellipsoid);

  // Populate the root tile with metadata
  parseTilesetMetadata(
      pLogger,
      tileUrl,
      std::move(tilesetGltf),
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
      std::make_shared<TilesetContentLoaderResult<TilesetGltfLoader>>(
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

TileLoadResult parseJsonContentInWorkerThread(
    const glm::dmat4& tileTransform,
    CesiumGeometry::Axis upAxis,
    Cesium3DTiles::TileRefine tileRefine,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest,
    ExternalContentInitializer&& externalContentInitializer,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const IntrusivePointer<CesiumGltf::Schema>& /*pExternalSchema*/) {
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  const auto& responseData = pResponse->data();

  CesiumGltfReader::GltfReader gltfReader;
  CesiumGltfReader::GltfReaderResult result = gltfReader.readGltf(responseData);
  if (!result.model) {
    SPDLOG_LOGGER_ERROR(pLogger, "Error(s) when parsing tileset glTF:");
    for (const std::string& error : result.errors) {
      SPDLOG_LOGGER_ERROR(pLogger, error);
    }
  }

  if (result.warnings.size() > 0) {
    SPDLOG_LOGGER_ERROR(pLogger, "Warning(s) when parsing tileset glTF:");
    for (const std::string& warning : result.warnings) {
      SPDLOG_LOGGER_ERROR(pLogger, warning);
    }
  }

  return parseExternalTilesetInWorkerThread(
      tileTransform,
      upAxis,
      tileRefine,
      pLogger,
      pAssetAccessor,
      std::move(pCompletedRequest),
      std::move(externalContentInitializer),
      ellipsoid,
      std::move(*result.model));
}
} // namespace

TilesetGltfLoader::TilesetGltfLoader(
    const std::string& baseUrl,
    CesiumGeometry::Axis upAxis,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _baseUrl{baseUrl}, _ellipsoid{ellipsoid}, _upAxis{upAxis}, _children{} {}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetGltfLoader>>
TilesetGltfLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& gltfUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  return externals.pAssetAccessor
      ->get(externals.asyncSystem, gltfUrl, requestHeaders)
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
          TilesetContentLoaderResult<TilesetGltfLoader> result;
          result.errors.emplaceError(fmt::format(
              "Did not receive a valid response for tile content {}",
              tileUrl));
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          TilesetContentLoaderResult<TilesetGltfLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl));
          result.statusCode = statusCode;
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        std::span<const std::byte> data = pResponse->data();
        CesiumGltfReader::GltfReader reader;
        CesiumGltfReader::GltfReaderResult gltfResult = reader.readGltf(data);
        if (!gltfResult.model) {
          TilesetContentLoaderResult<TilesetGltfLoader> loaderResult;
          loaderResult.errors.errors = std::move(gltfResult.errors);
          loaderResult.errors.warnings = std::move(gltfResult.warnings);
          return asyncSystem.createResolvedFuture(std::move(loaderResult));
        }

        return TilesetGltfLoader::createLoader(
            asyncSystem,
            pAssetAccessor,
            pLogger,
            pCompletedRequest->url(),
            pCompletedRequest->headers(),
            std::move(*gltfResult.model),
            ellipsoid);
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<TilesetGltfLoader>>
TilesetGltfLoader::createLoader(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& gltfUrl,
    const CesiumAsync::HttpHeaders& requestHeaders,
    CesiumGltf::Model&& gltf,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  TilesetContentLoaderResult<TilesetGltfLoader> result = parseTilesetGltf(
      pLogger,
      gltfUrl,
      {requestHeaders.begin(), requestHeaders.end()},
      gltf,
      glm::dmat4(1.0),
      Cesium3DTiles::TileRefine::Replace,
      ellipsoid);

  if (!result.pRootTile) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  // A new root tile will be created to represent the tileset.json itself.
  // If the original root tile had 3DTILES_tileset_voxels, extract it here
  // so it can be transferred to the new root tile.
  std::optional<Extension3dTilesTilesetVoxels> maybeVoxelExtension;
  if (result.pRootTile->isExternalContent()) {
    Cesium3DTilesSelection::TileExternalContent* pExternalContent =
        result.pRootTile->getContent().getExternalContent();
    CESIUM_ASSERT(pExternalContent);

    if (auto* pVoxelExtension =
            pExternalContent->getExtension<Extension3dTilesTilesetVoxels>()) {
      maybeVoxelExtension = std::move(*pVoxelExtension);
      pExternalContent->removeExtension<Extension3dTilesTilesetVoxels>();
    }
  }

  std::vector<Tile> children;
  children.emplace_back(std::move(*result.pRootTile));

  auto pContent = std::make_unique<TileExternalContent>();
  if (maybeVoxelExtension) {
    pContent->extensions.emplace(
        Extension3dTilesTilesetVoxels::ExtensionName,
        std::move(*maybeVoxelExtension));
  }

  result.pRootTile = std::make_unique<Tile>(
      children[0].getLoader(),
      TileID(),
      std::move(pContent));

  result.pRootTile->setTransform(children[0].getTransform());
  result.pRootTile->setBoundingVolume(children[0].getBoundingVolume());
  result.pRootTile->setUnconditionallyRefine();
  result.pRootTile->setRefine(children[0].getRefine());
  result.pRootTile->createChildTiles(std::move(children));

  // Populate the root tile with metadata
  TileExternalContent* pExternal =
      result.pRootTile->getContent().getExternalContent();
  CESIUM_ASSERT(pExternal);
  std::string externalSchemaUrl;
  if (pExternal) {
    parseTilesetMetadata(pLogger, gltfUrl, std::move(gltf), *pExternal);
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaderVector(
      requestHeaders.begin(),
      requestHeaders.end());
  return asyncSystem.createResolvedFuture(std::move(result))
      .thenInWorkerThread(
          [asyncSystem, pAssetAccessor](
              TilesetContentLoaderResult<TilesetGltfLoader>&& result) {
            TileExternalContent* pExternal =
                result.pRootTile->getContent().getExternalContent();
            CESIUM_ASSERT(pExternal);

            if (!pExternal->hasExtension<Extension3dTilesTilesetVoxels>()) {
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            // 3DTILES_content_voxels requires the tileset's schema to be
            // loaded.
            TilesetMetadata& metadata = pExternal->metadata;
            if (!metadata.schemaUri) {
              // No schema URI, so this is ready to go.
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            // Otherwise, prompt the schema to load from URI.
            return metadata.loadSchemaUri(asyncSystem, pAssetAccessor)
                .thenImmediately([result = std::move(result)]() mutable {
                  return std::move(result);
                });
          });
}

CesiumAsync::Future<TileLoadResult>
TilesetGltfLoader::loadTileContent(const TileLoadInput& loadInput) {
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
  Cesium3DTiles::TileRefine tileRefine = tile.getRefine();

  ExternalContentInitializer externalContentInitializer{nullptr, this, {}};

  const auto& ellipsoid = this->_ellipsoid;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& pSharedAssetSystem = loadInput.pSharedAssetSystem;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;
  const auto& pExternalSchema = this->_pExternalSchema;

  // If the URL is empty, this tile is empty content and we don't need to make a
  // web request to complete the loading process (in fact, a web request would
  // produce incorrect results as it would just be a request for _baseUrl).
  if (url->empty()) {
    return loadInput.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileEmptyContent{},
            this->_upAxis,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            pAssetAccessor,
            nullptr,
            {},
            TileLoadResultState::Success,
            ellipsoid});
  }

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
           pSharedAssetSystem,
           requestHeaders,
           pExternalSchema](std::shared_ptr<CesiumAsync::IAssetRequest>&&
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
            // TODO: can simplify since this is glTF?
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
              CesiumGltfReader::GltfReaderOptions gltfOptions =
                  contentOptions.toGltfReaderOptions();
              if (pSharedAssetSystem) {
                gltfOptions.pSharedAssetSystem = pSharedAssetSystem;
              }
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
              // Not renderable content, then it must be external tileset.
              return asyncSystem.createResolvedFuture(
                  parseJsonContentInWorkerThread(
                      tileTransform,
                      upAxis,
                      tileRefine,
                      pLogger,
                      pAssetAccessor,
                      std::move(pCompletedRequest),
                      std::move(externalContentInitializer),
                      ellipsoid,
                      pExternalSchema));
            }
          });
}

TileChildrenResult TilesetGltfLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  if (pLoader != this) {
    return pLoader->createTileChildren(tile, ellipsoid);
  }

  return {{}, TileLoadResultState::Failed};
}

const std::string& TilesetGltfLoader::getBaseUrl() const noexcept {
  return this->_baseUrl;
}

CesiumGeometry::Axis TilesetGltfLoader::getUpAxis() const noexcept {
  return _upAxis;
}

const CesiumUtility::IntrusivePointer<CesiumGltf::Schema>&
TilesetGltfLoader::getExternalSchema() const noexcept {
  return _pExternalSchema;
}

void TilesetGltfLoader::addChildLoader(
    std::unique_ptr<TilesetContentLoader> pLoader) {
  if (this->getOwner() != nullptr) {
    pLoader->setOwner(*this->getOwner());
  }
  pLoader->setExternalSchema(this->_pExternalSchema.get());
  this->_children.emplace_back(std::move(pLoader));
}

void TilesetGltfLoader::setOwnerOfNestedLoaders(
    TilesetContentManager& owner) noexcept {
  for (const std::unique_ptr<TilesetContentLoader>& pLoader : this->_children) {
    pLoader->setOwner(owner);
  }
}

void TilesetGltfLoader::setExternalSchema(CesiumGltf::Schema* schema) {
  this->_pExternalSchema = schema;
}

CesiumUtility::IntrusivePointer<CesiumGltf::Schema>
TilesetGltfLoader::getExternalSchema() {
  return this->_pExternalSchema;
}
} // namespace Cesium3DTilesSelection
