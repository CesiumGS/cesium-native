#include "TilesetLoadTileFromJson.h"

#include "Cesium3DTilesSelection/BoundingVolume.h"

#include <CesiumUtility/JsonHelpers.h>

#include <rapidjson/document.h>

#include <cctype>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {

std::optional<BoundingVolume> getBoundingVolumeProperty(
    const rapidjson::Value& tileJson,
    const std::string& key);

void parseImplicitTileset(
    Tile& tile,
    const rapidjson::Value& tileJson,
    const char* contentUri,
    const TileContext& context,
    std::vector<std::unique_ptr<TileContext>>& newContexts);

} // namespace

void Tileset::LoadTileFromJson::execute(
    Tile& tile,
    std::vector<std::unique_ptr<TileContext>>& newContexts,
    const rapidjson::Value& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    const TileContext& context,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  if (!tileJson.IsObject()) {
    return;
  }

  tile.setContext(const_cast<TileContext*>(&context));

  const std::optional<glm::dmat4x4> tileTransform =
      JsonHelpers::getTransformProperty(tileJson, "transform");
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

  const BoundingRegion* pRegion = std::get_if<BoundingRegion>(&*boundingVolume);
  const BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<BoundingRegionWithLooseFittingHeights>(&*boundingVolume);

  if (!pRegion && pLooseRegion) {
    pRegion = &pLooseRegion->getBoundingRegion();
  }

  std::optional<double> geometricError =
      JsonHelpers::getScalarProperty(tileJson, "geometricError");
  if (!geometricError) {
    geometricError = tile.getNonZeroGeometricError();
    SPDLOG_LOGGER_ERROR(
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
      parseImplicitTileset(tile, tileJson, contentUri, context, newContexts);
    }
  } else if (childrenIt->value.IsArray()) {
    const auto& childrenJson = childrenIt->value;
    tile.createChildTiles(childrenJson.Size());
    const gsl::span<Tile> childTiles = tile.getChildren();

    for (rapidjson::SizeType i = 0; i < childrenJson.Size(); ++i) {
      const auto& childJson = childrenJson[i];
      Tile& child = childTiles[i];
      child.setParent(&tile);
      LoadTileFromJson::execute(
          child,
          newContexts,
          childJson,
          transform,
          tile.getRefine(),
          context,
          pLogger);
    }
  }
}

namespace {

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
      std::string token =
          JsonHelpers::getStringOrDefault(s2It->value, "token", "1");
      double minimumHeight =
          JsonHelpers::getDoubleOrDefault(s2It->value, "minimumHeight", 0.0);
      double maximumHeight =
          JsonHelpers::getDoubleOrDefault(s2It->value, "maximumHeight", 0.0);
      return S2CellBoundingVolume(
          S2CellID::fromToken(token),
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
    return OrientedBoundingBox(
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
    return BoundingRegion(
        GlobeRectangle(
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
    return BoundingSphere(
        glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
        a[3].GetDouble());
  }

  return std::nullopt;
}

void parseImplicitTileset(
    Tile& tile,
    const rapidjson::Value& tileJson,
    const char* contentUri,
    const TileContext& context,
    std::vector<std::unique_ptr<TileContext>>& newContexts) {

  auto extensionsIt = tileJson.FindMember("extensions");
  if (extensionsIt != tileJson.MemberEnd() && extensionsIt->value.IsObject()) {
    auto extension = extensionsIt->value.GetObject();
    auto implicitTilingIt = extension.FindMember("3DTILES_implicit_tiling");
    if (implicitTilingIt != extension.MemberEnd() &&
        implicitTilingIt->value.IsObject()) {
      auto implicitTiling = implicitTilingIt->value.GetObject();

      auto tilingSchemeIt = implicitTiling.FindMember("subdivisionScheme");
      auto subtreeLevelsIt = implicitTiling.FindMember("subtreeLevels");
      auto maximumLevelIt = implicitTiling.FindMember("maximumLevel");
      auto subtreesIt = implicitTiling.FindMember("subtrees");

      if (tilingSchemeIt == implicitTiling.MemberEnd() ||
          !tilingSchemeIt->value.IsString() ||
          subtreeLevelsIt == implicitTiling.MemberEnd() ||
          !subtreeLevelsIt->value.IsUint() ||
          maximumLevelIt == implicitTiling.MemberEnd() ||
          !maximumLevelIt->value.IsUint() ||
          subtreesIt == implicitTiling.MemberEnd() ||
          !subtreesIt->value.IsObject()) {
        return;
      }

      uint32_t subtreeLevels = subtreeLevelsIt->value.GetUint();
      uint32_t maximumLevel = maximumLevelIt->value.GetUint();

      auto subtrees = subtreesIt->value.GetObject();
      auto subtreesUriIt = subtrees.FindMember("uri");

      if (subtreesUriIt == subtrees.MemberEnd() ||
          !subtreesUriIt->value.IsString()) {
        return;
      }

      const BoundingVolume& boundingVolume = tile.getBoundingVolume();
      const BoundingRegion* pRegion =
          std::get_if<BoundingRegion>(&boundingVolume);
      const OrientedBoundingBox* pBox =
          std::get_if<OrientedBoundingBox>(&boundingVolume);
      const S2CellBoundingVolume* pS2Cell =
          std::get_if<S2CellBoundingVolume>(&boundingVolume);

      ImplicitTilingContext implicitContext{
          {contentUri},
          subtreesUriIt->value.GetString(),
          std::nullopt,
          std::nullopt,
          boundingVolume,
          GeographicProjection(),
          std::nullopt,
          std::nullopt,
          std::nullopt};

      TileID rootID = "";

      const char* tilingScheme = tilingSchemeIt->value.GetString();
      if (!std::strcmp(tilingScheme, "QUADTREE")) {
        rootID = QuadtreeTileID(0, 0, 0);
        if (pRegion) {
          implicitContext.quadtreeTilingScheme = QuadtreeTilingScheme(
              projectRectangleSimple(
                  *implicitContext.projection,
                  pRegion->getRectangle()),
              1,
              1);
        } else if (pBox) {
          const glm::dvec3& boxLengths = pBox->getLengths();
          implicitContext.quadtreeTilingScheme = QuadtreeTilingScheme(
              CesiumGeometry::Rectangle(
                  -0.5 * boxLengths.x,
                  -0.5 * boxLengths.y,
                  0.5 * boxLengths.x,
                  0.5 * boxLengths.y),
              1,
              1);
        } else if (!pS2Cell) {
          return;
        }

        implicitContext.quadtreeAvailability =
            QuadtreeAvailability(subtreeLevels, maximumLevel);

      } else if (!std::strcmp(tilingScheme, "OCTREE")) {
        rootID = OctreeTileID(0, 0, 0, 0);
        if (pRegion) {
          implicitContext.octreeTilingScheme = OctreeTilingScheme(
              projectRegionSimple(*implicitContext.projection, *pRegion),
              1,
              1,
              1);
        } else if (pBox) {
          const glm::dvec3& boxLengths = pBox->getLengths();
          implicitContext.octreeTilingScheme = OctreeTilingScheme(
              CesiumGeometry::AxisAlignedBox(
                  -0.5 * boxLengths.x,
                  -0.5 * boxLengths.y,
                  -0.5 * boxLengths.z,
                  0.5 * boxLengths.x,
                  0.5 * boxLengths.y,
                  0.5 * boxLengths.z),
              1,
              1,
              1);
        } else if (!pS2Cell) {
          return;
        }

        implicitContext.octreeAvailability =
            OctreeAvailability(subtreeLevels, maximumLevel);
      }

      std::unique_ptr<TileContext> pNewContext =
          std::make_unique<TileContext>();
      pNewContext->pTileset = context.pTileset;
      pNewContext->baseUrl = context.baseUrl;
      pNewContext->requestHeaders = context.requestHeaders;
      pNewContext->version = context.version;
      pNewContext->failedTileCallback = context.failedTileCallback;
      pNewContext->contextInitializerCallback =
          context.contextInitializerCallback;

      TileContext* pContext = pNewContext.get();
      newContexts.push_back(std::move(pNewContext));
      tile.setContext(pContext);

      if (implicitContext.quadtreeAvailability ||
          implicitContext.octreeAvailability) {
        pContext->implicitContext = std::make_optional<ImplicitTilingContext>(
            std::move(implicitContext));

        // This will act as a dummy tile representing the implicit tileset. Its
        // only child will act as the actual root content of the new tileset.
        tile.createChildTiles(1);

        Tile& childTile = tile.getChildren()[0];
        childTile.setContext(pContext);
        childTile.setParent(&tile);
        childTile.setTileID(rootID);
        childTile.setBoundingVolume(tile.getBoundingVolume());
        childTile.setGeometricError(tile.getGeometricError());
        childTile.setRefine(tile.getRefine());

        tile.setUnconditionallyRefine();
      }

      // Don't try to load content for this tile.
      tile.setTileID("");
      tile.setEmptyContent();
    }
  }

  return;
}

} // namespace
