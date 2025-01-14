#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/ITilesetHeightSampler.h>
#include <Cesium3DTilesSelection/SampleHeightResult.h>
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
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/calcQuadtreeMaxGeometricError.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/JsonValue.h>

#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/matrix.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace Cesium3DTilesContent;

namespace Cesium3DTilesSelection {
EllipsoidTilesetLoader::EllipsoidTilesetLoader(const Ellipsoid& ellipsoid)
    : _projection(ellipsoid),
      _tilingScheme(
          _projection.project(_projection.MAXIMUM_GLOBE_RECTANGLE),
          2,
          1) {}

/*static*/ std::unique_ptr<Tileset> EllipsoidTilesetLoader::createTileset(
    const TilesetExternals& externals,
    const TilesetOptions& options) {
  std::unique_ptr<EllipsoidTilesetLoader> pCustomLoader =
      std::make_unique<EllipsoidTilesetLoader>(options.ellipsoid);
  std::unique_ptr<Tile> pRootTile =
      std::make_unique<Tile>(pCustomLoader.get(), TileEmptyContent{});

  pRootTile->setRefine(TileRefine::Replace);
  pRootTile->setUnconditionallyRefine();

  std::vector<Tile> children;
  uint32_t rootTilesX = pCustomLoader->_tilingScheme.getRootTilesX();
  children.reserve(rootTilesX);

  for (uint32_t x = 0; x < rootTilesX; x++) {
    pCustomLoader->createChildTile(
        *pRootTile,
        children,
        QuadtreeTileID{0, x, 0});
  }

  pRootTile->createChildTiles(std::move(children));

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
      input.pAssetAccessor,
      nullptr,
      {},
      TileLoadResultState::Success});
}

TileChildrenResult EllipsoidTilesetLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& /*ellipsoid*/) {
  const QuadtreeTileID* pParentID =
      std::get_if<QuadtreeTileID>(&tile.getTileID());

  // Due to the use of uint32_t for QuadtreeTileID X and Y, we can only support
  // through level 30.
  if (pParentID && pParentID->level < 30) {
    std::vector<Tile> children;
    QuadtreeChildren childIDs =
        ImplicitTilingUtilities::getChildren(*pParentID);
    children.reserve(childIDs.size());

    for (const QuadtreeTileID& childID : childIDs) {
      createChildTile(tile, children, childID);
    }

    return TileChildrenResult{
        std::move(children),
        TileLoadResultState::Success};
  }

  return TileChildrenResult{{}, TileLoadResultState::Failed};
}

ITilesetHeightSampler* EllipsoidTilesetLoader::getHeightSampler() {
  return this;
}

CesiumAsync::Future<SampleHeightResult> EllipsoidTilesetLoader::sampleHeights(
    const CesiumAsync::AsyncSystem& asyncSystem,
    std::vector<CesiumGeospatial::Cartographic>&& positions) {
  SampleHeightResult result;

  result.positions = std::move(positions);
  result.sampleSuccess.resize(result.positions.size(), true);

  for (Cartographic& position : result.positions) {
    position.height = 0.0;
  }

  return asyncSystem.createResolvedFuture(std::move(result));
}

void EllipsoidTilesetLoader::createChildTile(
    const Tile& parent,
    std::vector<Tile>& children,
    const QuadtreeTileID& childID) const {
  BoundingRegion boundingRegion = createBoundingRegion(childID);
  const GlobeRectangle& globeRectangle = boundingRegion.getRectangle();

  Tile& child = children.emplace_back(parent.getLoader());
  child.setTileID(childID);
  child.setRefine(parent.getRefine());
  child.setTransform(glm::translate(
      glm::dmat4x4(1.0),
      _projection.getEllipsoid().cartographicToCartesian(
          globeRectangle.getNorthwest())));
  child.setBoundingVolume(boundingRegion);
  child.setGeometricError(
      8.0 * calcQuadtreeMaxGeometricError(_projection.getEllipsoid()) *
      globeRectangle.computeWidth());
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
  static constexpr size_t resolution = 24;

  std::vector<uint16_t> indices;
  indices.reserve(6 * (resolution - 1) * (resolution - 1));

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

      uint16_t index = static_cast<uint16_t>((resolution * x) + y);
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

  model.asset.version = "2.0";
  model.extras["gltfUpAxis"] = JsonValue(std::underlying_type_t<Axis>(Axis::Z));

  model.buffers.resize(1);
  model.bufferViews.resize(3);
  model.accessors.resize(3);
  model.materials.resize(1);
  model.meshes.resize(1);
  model.scenes.resize(1);
  model.nodes.resize(1);

  model.meshes[0].primitives.resize(1);
  model.scenes[0].nodes.emplace_back(0);
  model.nodes[0].mesh = 0;

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
  bufferViewIndices.byteLength = static_cast<int64_t>(indicesSize);
  bufferViewIndices.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  BufferView& bufferViewVertices = model.bufferViews[1];
  bufferViewVertices.buffer = 0;
  bufferViewVertices.byteOffset = static_cast<int64_t>(indicesSize);
  bufferViewVertices.byteLength = static_cast<int64_t>(verticesSize);
  bufferViewVertices.target = BufferView::Target::ARRAY_BUFFER;

  BufferView& bufferViewNormals = model.bufferViews[2];
  bufferViewNormals.buffer = 0;
  bufferViewNormals.byteOffset =
      static_cast<int64_t>(indicesSize + verticesSize);
  bufferViewNormals.byteLength = static_cast<int64_t>(normalsSize);
  bufferViewNormals.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& accessorIndices = model.accessors[0];
  accessorIndices.bufferView = 0;
  accessorIndices.count = static_cast<int64_t>(indices.size());
  accessorIndices.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  accessorIndices.type = Accessor::Type::SCALAR;

  Accessor& accessorVertices = model.accessors[1];
  accessorVertices.bufferView = 1;
  accessorVertices.count = static_cast<int64_t>(vertices.size());
  accessorVertices.componentType = Accessor::ComponentType::FLOAT;
  accessorVertices.type = Accessor::Type::VEC3;

  Accessor& accessorNormals = model.accessors[2];
  accessorNormals.bufferView = 2;
  accessorNormals.count = static_cast<int64_t>(normals.size());
  accessorNormals.componentType = Accessor::ComponentType::FLOAT;
  accessorNormals.type = Accessor::Type::VEC3;

  MeshPrimitive& primitive = model.meshes[0].primitives[0];
  primitive.attributes["POSITION"] = 1;
  primitive.attributes["NORMAL"] = 2;
  primitive.indices = 0;
  primitive.material = 0;

  return model;
}
} // namespace Cesium3DTilesSelection
