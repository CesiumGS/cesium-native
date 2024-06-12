#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/calcQuadtreeMaxGeometricError.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorSpec.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferCesium.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Scene.h>

#include <glm/detail/type_vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

using namespace Cesium3DTilesContent;

namespace Cesium3DTilesSelection {
EllipsoidTilesetLoader::EllipsoidTilesetLoader(const Ellipsoid& ellipsoid)
    : _projection(ellipsoid),
      _tilingScheme(
          _projection.project(_projection.MAXIMUM_GLOBE_RECTANGLE),
          1,
          1) {}

/*static*/ std::unique_ptr<Tileset> EllipsoidTilesetLoader::createTileset(
    const TilesetExternals& externals,
    const Ellipsoid& ellipsoid,
    const TilesetOptions& options) {
  std::unique_ptr<TilesetContentLoader> pCustomLoader =
      std::make_unique<EllipsoidTilesetLoader>(ellipsoid);
  std::unique_ptr<Tile> pRootTile =
      std::make_unique<Tile>(pCustomLoader.get(), TileEmptyContent{});

  pRootTile->setTileID(QuadtreeTileID{0, 0, 0});
  pRootTile->setRefine(TileRefine::Replace);
  pRootTile->setUnconditionallyRefine();

  return std::make_unique<Tileset>(
      externals,
      std::move(pCustomLoader),
      std::move(pRootTile),
      options);
}

Future<TileLoadResult>
EllipsoidTilesetLoader::loadTileContent(const TileLoadInput& input) {
  return input.asyncSystem.createResolvedFuture(TileLoadResult{
      createModel(createGeometry(input.tile)),
      Axis::Z,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      nullptr,
      {},
      TileLoadResultState::Success});
}

TileChildrenResult
EllipsoidTilesetLoader::createTileChildren(const Tile& tile) {
  const QuadtreeTileID& parentID = std::get<QuadtreeTileID>(tile.getTileID());

  QuadtreeChildren childIDs = ImplicitTilingUtilities::getChildren(parentID);

  std::vector<Tile> children;
  children.reserve(childIDs.size());

  for (const QuadtreeTileID& childID : childIDs) {
    Tile& child = children.emplace_back(this);

    BoundingRegion boundingRegion = createBoundingRegion(childID);
    const GlobeRectangle& globeRectangle = boundingRegion.getRectangle();

    child.setTileID(childID);
    child.setRefine(tile.getRefine());
    child.setTransform(glm::translate(
        glm::dmat4x4(1.0),
        _projection.getEllipsoid().cartographicToCartesian(
            globeRectangle.getNorthwest())));
    child.setBoundingVolume(boundingRegion);
    child.setGeometricError(
        4.0 * calcQuadtreeMaxGeometricError(_projection.getEllipsoid()) *
        globeRectangle.computeWidth());
  }

  return TileChildrenResult{std::move(children), TileLoadResultState::Success};
}

BoundingRegion EllipsoidTilesetLoader::createBoundingRegion(
    const QuadtreeTileID& quadtreeID) const {
  return BoundingRegion(
      _projection.unproject(_tilingScheme.tileToRectangle(quadtreeID)),
      0.0,
      0.0,
      _projection.getEllipsoid());
}

EllipsoidTilesetLoader::Geometry
EllipsoidTilesetLoader::createGeometry(const Tile& tile) const {
  static constexpr uint16_t resolution = 32;

  std::vector<uint16_t> indices(6 * (resolution - 1) * (resolution - 1));
  std::vector<glm::vec3> vertices(resolution * resolution);
  std::vector<glm::vec3> normals(vertices.size());

  const Ellipsoid& ellipsoid = _projection.getEllipsoid();
  const GlobeRectangle& rectangle =
      std::get<BoundingRegion>(tile.getBoundingVolume()).getRectangle();

  double west = rectangle.getWest();
  double east = rectangle.getEast();
  double north = rectangle.getNorth();
  double south = rectangle.getSouth();

  double lonStep = (east - west) / (resolution - 1);
  double latStep = (south - north) / (resolution - 1);

  glm::dmat4 inverseTransform = glm::inverse(tile.getTransform());

  for (uint16_t x = 0; x < resolution; x++) {
    double longitude = (lonStep * x) + west;
    for (uint16_t y = 0; y < resolution; y++) {
      double latitude = (latStep * y) + north;
      Cartographic cartographic(longitude, latitude);

      uint16_t index = (resolution * x) + y;
      vertices[index] = glm::dvec3(
          inverseTransform *
          glm::dvec4(ellipsoid.cartographicToCartesian(cartographic), 1.0));
      normals[index] = ellipsoid.geodeticSurfaceNormal(cartographic);

      if (x < resolution - 1 && y < resolution - 1) {
        uint16_t a = index + 1;
        uint16_t b = index + resolution;
        uint16_t c = b + 1;
        indices.insert(indices.end(), {b, index, a, b, a, c});
      }
    }
  }

  return Geometry{std::move(indices), std::move(vertices), std::move(normals)};
}

Model EllipsoidTilesetLoader::createModel(const Geometry& geometry) const {
  const std::vector<uint16_t>& indices = geometry.indices;
  const std::vector<glm::vec3>& vertices = geometry.vertices;
  const std::vector<glm::vec3>& normals = geometry.normals;

  size_t indicesSize = indices.size() * sizeof(uint16_t);
  size_t verticesSize = vertices.size() * sizeof(glm::vec3);
  size_t normalsSize = verticesSize;

  Model model;
  model.buffers.resize(1);
  model.bufferViews.resize(3);
  model.accessors.resize(3);
  model.materials.resize(1);
  model.meshes.resize(1);
  model.scenes.resize(1);
  model.nodes.resize(1);

  std::vector<std::byte>& buffer = model.buffers[0].cesium.data;
  buffer.resize(indicesSize + verticesSize + normalsSize);
  std::memcpy(buffer.data(), indices.data(), indicesSize);
  std::memcpy(buffer.data() + indicesSize, vertices.data(), verticesSize);
  std::memcpy(
      buffer.data() + indicesSize + verticesSize,
      normals.data(),
      normalsSize);

  BufferView& bufferViewIndices = model.bufferViews[0];
  bufferViewIndices.buffer = 0;
  bufferViewIndices.byteOffset = 0;
  bufferViewIndices.byteLength = indicesSize;
  bufferViewIndices.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  BufferView& bufferViewVertices = model.bufferViews[1];
  bufferViewVertices.buffer = 0;
  bufferViewVertices.byteOffset = indicesSize;
  bufferViewVertices.byteLength = verticesSize;
  bufferViewVertices.target = BufferView::Target::ARRAY_BUFFER;

  BufferView& bufferViewNormals = model.bufferViews[2];
  bufferViewNormals.buffer = 0;
  bufferViewNormals.byteOffset = indicesSize + verticesSize;
  bufferViewNormals.byteLength = normalsSize;
  bufferViewNormals.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& accessorIndices = model.accessors[0];
  accessorIndices.bufferView = 0;
  accessorIndices.count = indices.size();
  accessorIndices.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  accessorIndices.type = Accessor::Type::SCALAR;

  Accessor& accessorVertices = model.accessors[1];
  accessorVertices.bufferView = 1;
  accessorVertices.count = vertices.size();
  accessorVertices.componentType = Accessor::ComponentType::FLOAT;
  accessorVertices.type = Accessor::Type::VEC3;

  Accessor& accessorNormals = model.accessors[2];
  accessorNormals.bufferView = 2;
  accessorNormals.count = normals.size();
  accessorNormals.componentType = Accessor::ComponentType::FLOAT;
  accessorNormals.type = Accessor::Type::VEC3;

  MeshPrimitive primitive;
  primitive.attributes["POSITION"] = 1;
  primitive.attributes["NORMAL"] = 2;
  primitive.indices = 0;
  primitive.material = 0;

  model.meshes[0].primitives.emplace_back(std::move(primitive));
  model.nodes[0].mesh = 0;
  model.scenes[0].nodes.emplace_back(0);

  return model;
}
} // namespace Cesium3DTilesSelection