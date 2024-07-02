#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltf/Model.h>

#include <glm/fwd.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace Cesium3DTilesSelection {
class EllipsoidTilesetLoader : public TilesetContentLoader {
public:
  EllipsoidTilesetLoader(
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  static std::unique_ptr<Tileset> createTileset(
      const TilesetExternals& externals,
      const TilesetOptions& options = TilesetOptions{});

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) override;
  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) override;

private:
  struct Geometry {
    std::vector<uint16_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
  };

  void createChildTile(
      const Tile& parent,
      std::vector<Tile>& children,
      const CesiumGeometry::QuadtreeTileID& childID) const;

  CesiumGeospatial::BoundingRegion
  createBoundingRegion(const CesiumGeometry::QuadtreeTileID& quadtreeID) const;
  Geometry createGeometry(const Tile& tile) const;
  CesiumGltf::Model createModel(const Geometry& geometry) const;

  CesiumGeospatial::GeographicProjection _projection;
  CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
};
} // namespace Cesium3DTilesSelection
