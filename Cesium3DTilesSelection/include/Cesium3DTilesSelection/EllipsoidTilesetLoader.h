#pragma once

#include <Cesium3DTilesSelection/ITilesetHeightSampler.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>

namespace Cesium3DTilesSelection {
/**
 * @brief A loader that will generate a tileset by tesselating the surface of an
 * ellipsoid, producing a simple globe tileset without any terrain features.
 */
class EllipsoidTilesetLoader : public TilesetContentLoader,
                               public ITilesetHeightSampler {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   */
  EllipsoidTilesetLoader(
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Creates a new tileset with this loader.
   *
   * @param externals The external interfaces to use.
   * @param options Additional options for the tileset.
   */
  static std::unique_ptr<Tileset> createTileset(
      const TilesetExternals& externals,
      const TilesetOptions& options = TilesetOptions{});

  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) override;
  TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) override;

  ITilesetHeightSampler* getHeightSampler() override;

  CesiumAsync::Future<SampleHeightResult> sampleHeights(
      const CesiumAsync::AsyncSystem& asyncSystem,
      std::vector<CesiumGeospatial::Cartographic>&& positions) override;

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
