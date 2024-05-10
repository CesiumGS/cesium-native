#include "PxConfig.h"
#include "PxPhysicsAPI.h"

#include <Cesium3DTilesReader/TilesetReader.h>
#include <CesiumAnalysis/Physics.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/readFile.h>

#include <catch2/catch.hpp>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesReader;

TEST_CASE("Physics") {
  CesiumAnalysis::Physics physics;

  std::vector<std::byte> glbData = readFile("some-tile.glb");
  CesiumGltfReader::GltfReader reader{};
  CesiumGltfReader::GltfReaderResult result = reader.readGltf(glbData);
  REQUIRE(result.model);

  physx::PxDefaultMemoryOutputStream writeBuffer;
  physics.cookTriangleMesh(writeBuffer, *result.model);

  physx::PxDefaultMemoryInputData readBuffer(
      writeBuffer.getData(),
      writeBuffer.getSize());
  physx::PxTriangleMesh* pMesh = physics.createTriangleMesh(readBuffer);
  REQUIRE(pMesh);

  physx::PxVec3 origin(1.0, 2.0, 3.0);
  physx::PxVec3 direction(1.0, 0.0, 0.0);
  physx::PxTransform pose{physx::PxIdentity};
  physx::PxGeomRaycastHit hits[10];
  physx::PxU32 numHits = physx::PxGeometryQuery::raycast(
      origin,
      direction,
      physx::PxTriangleMeshGeometry(pMesh),
      pose,
      100.0,
      physx::PxHitFlag::ePOSITION,
      10,
      hits,
      sizeof(physx::PxGeomRaycastHit));
  CHECK(numHits > 0);
}
