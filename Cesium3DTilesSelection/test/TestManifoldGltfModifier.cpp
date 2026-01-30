#include "CesiumAsync/AsyncSystem.h"
#include "CesiumNativeTests/SimpleAssetAccessor.h"
#include "CesiumNativeTests/SimpleTaskProcessor.h"

#include <Cesium3DTilesSelection/ManifoldGltfModifier.h>
#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGltfWriter/GltfWriter.h>

#include <doctest/doctest.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <memory>

using namespace CesiumGltf;
using namespace CesiumGeometry;
using namespace Cesium3DTilesSelection;

namespace {
CesiumGltf::Model createBox(const CesiumGeometry::AxisAlignedBox& box) {
  std::vector<glm::vec3> vertices{
      glm::vec3(box.minimumX, box.minimumY, box.minimumZ),
      glm::vec3(box.maximumX, box.minimumY, box.minimumZ),
      glm::vec3(box.maximumX, box.maximumY, box.minimumZ),
      glm::vec3(box.minimumX, box.maximumY, box.minimumZ),
      glm::vec3(box.minimumX, box.minimumY, box.maximumZ),
      glm::vec3(box.maximumX, box.minimumY, box.maximumZ),
      glm::vec3(box.maximumX, box.maximumY, box.maximumZ),
      glm::vec3(box.minimumX, box.maximumY, box.maximumZ)};

  std::vector<glm::vec2> texcoords {
    glm::vec2(0, 0),
    glm::vec2(1, 0),
    glm::vec2(1, 1),
    glm::vec2(0, 1),
    glm::vec2(0, 0),
    glm::vec2(1, 0),
    glm::vec2(1, 1),
    glm::vec2(0, 1)
  };

  std::vector<uint32_t> indices{1, 5, 0, 0, 5, 4, 6, 2, 7, 7, 2, 3, 3, 0, 7, 7, 0, 4, 7, 4, 6, 6, 4, 5, 6, 5, 2, 2, 5, 1, 2, 1, 3, 3, 1, 0};

  const size_t verticesSize = vertices.size() * sizeof(glm::vec3) + texcoords.size() * sizeof(glm::vec2);
  const size_t indicesSize = indices.size() * sizeof(uint32_t);

  CesiumGltf::Model model;
  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(verticesSize + indicesSize);
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

  std::memcpy(buffer.cesium.data.data(), vertices.data(), vertices.size() * sizeof(glm::vec3));
  std::memcpy(buffer.cesium.data.data() + (vertices.size() * sizeof(glm::vec3)), texcoords.data(), texcoords.size() * sizeof(glm::vec2));

  std::memcpy(
      buffer.cesium.data.data() + verticesSize,
      indices.data(),
      indicesSize);

  int32_t verticesBufferViewIdx = (int32_t)model.bufferViews.size();
  BufferView& verticesBufferView = model.bufferViews.emplace_back();
  verticesBufferView.buffer = 0;
  verticesBufferView.byteOffset = 0;
  verticesBufferView.target = BufferView::Target::ARRAY_BUFFER;
  verticesBufferView.byteLength = (int64_t)(vertices.size() * sizeof(glm::vec3));

  int32_t positionAccessorIdx = (int32_t)model.accessors.size();
  Accessor& positionAccessor = model.accessors.emplace_back();
  positionAccessor.bufferView = verticesBufferViewIdx;
  positionAccessor.byteOffset = 0;
  positionAccessor.count = (int64_t)vertices.size();
  positionAccessor.type = Accessor::Type::VEC3;
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.min =
      std::vector<double>{box.minimumX, box.minimumY, box.minimumZ};
  positionAccessor.max =
      std::vector<double>{box.maximumX, box.maximumY, box.maximumZ};

  int32_t texcoordsBufferViewIdx = (int32_t)model.bufferViews.size();
  BufferView& texcoordsBufferView = model.bufferViews.emplace_back();
  texcoordsBufferView.buffer = 0;
  texcoordsBufferView.byteOffset = (int64_t)(texcoords.size() * sizeof(glm::vec3));
  texcoordsBufferView.target = BufferView::Target::ARRAY_BUFFER;
  texcoordsBufferView.byteLength = (int64_t)(texcoords.size() * sizeof(glm::vec2));
  
  int32_t texcoordsAccessorIdx = (int32_t)model.accessors.size();
  Accessor& texcoordsAccessor = model.accessors.emplace_back();
  texcoordsAccessor.bufferView = texcoordsBufferViewIdx;
  texcoordsAccessor.byteOffset = 0;
  texcoordsAccessor.count = (int64_t)texcoords.size();
  texcoordsAccessor.type = Accessor::Type::VEC2;
  texcoordsAccessor.componentType = Accessor::ComponentType::FLOAT;

  int32_t indicesBufferViewIdx = (int32_t)model.bufferViews.size();
  BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = 0;
  indicesBufferView.byteOffset = (int64_t)verticesSize;
  indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;
  indicesBufferView.byteLength = (int64_t)indicesSize;

  int32_t indicesAccessorIdx = (int32_t)model.accessors.size();
  Accessor& indicesAccessor = model.accessors.emplace_back();
  indicesAccessor.bufferView = indicesBufferViewIdx;
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = (int32_t)indices.size();
  indicesAccessor.type = Accessor::Type::SCALAR;
  indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_INT;

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.indices = indicesAccessorIdx;
  primitive.attributes.emplace("POSITION", positionAccessorIdx);
  primitive.attributes.emplace("TEXCOORD_0", texcoordsAccessorIdx);

  return model;
}
} // namespace

TEST_CASE("ManifoldGltfModifier can divide a cube in half") {
  CesiumGltfWriter::GltfWriter writer;
  Model cubeModel = createBox(AxisAlignedBox(-1, -1, -1, 1, 1, 1));
  cubeModel.asset.version = "2.0";
  cubeModel.scene = 0;
  CesiumGltf::Node& node = cubeModel.nodes.emplace_back();
  node.mesh = 0;
  CesiumGltf::Scene& scene = cubeModel.scenes.emplace_back();
  scene.nodes.emplace_back(0);

  CesiumGltfWriter::GltfWriterResult result =
      writer.writeGlb(cubeModel, cubeModel.buffers[0].cesium.data);

  std::ofstream cubeFile("output-cube.glb", std::ios::out);
  cubeFile.write(
      reinterpret_cast<const char*>(result.gltfBytes.data()),
      (std::streamsize)result.gltfBytes.size());
  cubeFile.close();

  ManifoldGltfModifier modifier;
  modifier.box = AxisAlignedBox(-1, 0, -1, 1, 1, 1);

  CesiumAsync::AsyncSystem asyncSystem = CesiumAsync::AsyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
          std::map<
              std::string,
              std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>());

  GltfModifierInput input{
      0,
      asyncSystem,
      pAssetAccessor,
      spdlog::default_logger(),
      cubeModel,
      glm::dmat4()};

  CesiumAsync::Future<std::optional<GltfModifierOutput>> output =
      modifier.apply(std::move(input));

  while (!output.isReady()) {
    asyncSystem.dispatchMainThreadTasks();
  }

  std::optional<GltfModifierOutput> outputOpt = output.waitInMainThread();
  REQUIRE(outputOpt);

  result = writer.writeGlb(
      outputOpt->modifiedModel,
      outputOpt->modifiedModel.buffers[0].cesium.data);

  std::ofstream file("output-manifold.glb", std::ios::out);
  file.write(
      reinterpret_cast<const char*>(result.gltfBytes.data()),
      (std::streamsize)result.gltfBytes.size());
  file.close();
}