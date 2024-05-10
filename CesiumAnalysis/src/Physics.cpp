#include "PxConfig.h"
#include "PxPhysicsAPI.h"

#include <CesiumAnalysis/Physics.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Model.h>

using namespace physx;

namespace {
PxDefaultErrorCallback gDefaultErrorCallback;
PxDefaultAllocator gDefaultAllocatorCallback;
} // namespace

namespace CesiumAnalysis {

Physics::Physics() : _pFoundation(nullptr), _pPhysics(nullptr) {
  this->_pFoundation = PxCreateFoundation(
      PX_PHYSICS_VERSION,
      gDefaultAllocatorCallback,
      gDefaultErrorCallback);

  if (this->_pFoundation) {
    bool recordMemoryAllocations = true;
    this->_pPhysics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *this->_pFoundation,
        PxTolerancesScale(),
        recordMemoryAllocations,
        nullptr);
  }
}

Physics::~Physics() {
  this->_pPhysics->release();
  this->_pFoundation->release();
}

void Physics::cookTriangleMesh(
    physx::PxOutputStream& output,
    const CesiumGltf::Model& model) {
  // TODO: support multiple meshes / primitives
  assert(model.meshes.size() == 1);
  assert(model.meshes[0].primitives.size() >= 1);

  const CesiumGltf::MeshPrimitive& primitive = model.meshes[0].primitives[0];

  // TODO: support non-indexed primitives
  assert(primitive.indices >= 0);
  // TODO: support non-triangle primitives
  assert(primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES);

  auto positionIt = primitive.attributes.find("POSITION");
  assert(positionIt != primitive.attributes.end());

  const CesiumGltf::Accessor& indicesAccessor =
      model.getSafe(model.accessors, primitive.indices);
  const CesiumGltf::BufferView& indicesBufferView =
      model.getSafe(model.bufferViews, indicesAccessor.bufferView);
  const CesiumGltf::Buffer& indicesBuffer =
      model.getSafe(model.buffers, indicesBufferView.buffer);

  const CesiumGltf::Accessor& positionsAccessor =
      model.getSafe(model.accessors, positionIt->second);
  const CesiumGltf::BufferView& positionsBufferView =
      model.getSafe(model.bufferViews, positionsAccessor.bufferView);
  const CesiumGltf::Buffer& positionsBuffer =
      model.getSafe(model.buffers, positionsBufferView.buffer);

  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count = uint32_t(positionsAccessor.count);
  meshDesc.points.stride = uint32_t(positionsAccessor.computeByteStride(model));
  meshDesc.points.data = positionsBuffer.cesium.data.data() +
                         positionsBufferView.byteOffset +
                         positionsAccessor.byteOffset;

  meshDesc.triangles.count = uint32_t(indicesAccessor.count / 3);
  meshDesc.triangles.stride =
      uint32_t(3 * indicesAccessor.computeByteSizeOfComponent());
  meshDesc.triangles.data = indicesBuffer.cesium.data.data() +
                            indicesBufferView.byteOffset +
                            indicesAccessor.byteOffset;

  if (meshDesc.triangles.stride == 6) {
    meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
  }

  PxTolerancesScale scale;
  PxCookingParams params(scale);

  PxTriangleMeshCookingResult::Enum result;
  bool status = PxCookTriangleMesh(params, meshDesc, output, &result);
  if (!status) {
    throw std::exception("something went wrong");
  }
}

physx::PxTriangleMesh*
Physics::createTriangleMesh(physx::PxInputStream& cookedDataStream) {
  return this->_pPhysics->createTriangleMesh(cookedDataStream);
}

} // namespace CesiumAnalysis
