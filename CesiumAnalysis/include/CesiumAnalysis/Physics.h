#pragma once

namespace physx {
class PxFoundation;
class PxPhysics;
class PxOutputStream;
} // namespace physx

namespace CesiumGltf {
struct Model;
}

namespace CesiumAnalysis {

class Physics {
public:
  Physics();
  ~Physics();

  physx::PxFoundation* getFoundation() { return this->_pFoundation; }
  physx::PxPhysics* getPhysics() { return this->_pPhysics; };

  void cookTriangleMesh(
      physx::PxOutputStream& output,
      const CesiumGltf::Model& model);

  physx::PxTriangleMesh*
  createTriangleMesh(physx::PxInputStream& cookedDataStream);

private:
  physx::PxFoundation* _pFoundation;
  physx::PxPhysics* _pPhysics;
};

} // namespace CesiumAnalysis
