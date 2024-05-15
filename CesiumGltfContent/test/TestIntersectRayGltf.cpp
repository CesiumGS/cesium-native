#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/Ray.h"
#include "CesiumGltfContent/GltfUtilities.h"
#include "CesiumGltfReader/GltfReader.h"

#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>

using namespace CesiumUtility;
using namespace CesiumGltf;
using namespace CesiumGeometry;
using namespace CesiumGltfReader;
using namespace CesiumGltfContent;

namespace {
std::vector<std::byte> readFile(const std::filesystem::path& fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  REQUIRE(file);

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);

  return buffer;
}
} // namespace

void checkIntersection(
    const Ray& ray,
    Model& model,
    bool cullBackFaces,
    bool shouldHit,
    const glm::dvec3& expectedHit) {
  std::optional<GltfUtilities::HitResult> hitResult =
      GltfUtilities::intersectRayGltfModel(ray, model, cullBackFaces);

  if (!shouldHit) {
    CHECK(!hitResult.has_value());
    return;
  }

  // Validate hit point
  CHECK(hitResult.has_value());
  CHECK(glm::all(glm::lessThan(
      glm::abs(hitResult->point - expectedHit),
      glm::dvec3(CesiumUtility::Math::Epsilon6))));

  // Use results to dive into model
  CHECK(hitResult->meshId > -1);
  CHECK(static_cast<size_t>(hitResult->meshId) < model.meshes.size());
  CesiumGltf::Mesh& mesh = model.meshes[static_cast<size_t>(hitResult->meshId)];

  CHECK(hitResult->primitiveId > -1);
  CHECK(static_cast<size_t>(hitResult->primitiveId) < mesh.primitives.size());
  CesiumGltf::MeshPrimitive& primitive =
      mesh.primitives[static_cast<size_t>(hitResult->primitiveId)];

  bool modeIsValid =
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES ||
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP;
  CHECK(modeIsValid);

  // Returned matrix should be invertible
  CHECK(glm::determinant(hitResult->primitiveToWorld) != 0);

  // There should be positions
  auto positionAccessorIt = primitive.attributes.find("POSITION");
  CHECK(positionAccessorIt != primitive.attributes.end());

  // And a way to access them
  int positionAccessorID = positionAccessorIt->second;
  const Accessor* pPositionAccessor =
      Model::getSafe(&model.accessors, positionAccessorID);
  CHECK(pPositionAccessor);

  // This should have exactly 3 indices (or none if no index buffer)
  bool indicesAreValid = primitive.indices == 3 || primitive.indices == 0;
  CHECK(indicesAreValid);
}

TEST_CASE("GltfUtilities::intersectRayGltfModel") {

  GltfReader reader;
  Model cube = *reader
                    .readGltf(readFile(
                        std::filesystem::path(CesiumGltfContent_TEST_DATA_DIR) /
                        "cube.glb"))
                    .model;
  Model translatedCube =
      *reader
           .readGltf(readFile(
               std::filesystem::path(CesiumGltfContent_TEST_DATA_DIR) /
               "translated_cube.glb"))
           .model;
  Model sphere =
      *reader
           .readGltf(readFile(
               std::filesystem::path(CesiumGltfContent_TEST_DATA_DIR) /
               "sphere.glb"))
           .model;

  // intersects the top side of the cube
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      cube,
      true,
      true,
      glm::dvec3(0.0, 0.0, 1.0));

  // misses the top side of the cube
  checkIntersection(
      Ray(glm::dvec3(2.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      cube,
      true,
      false,
      {});

  // intersects a corner of the cube
  checkIntersection(
      Ray(glm::dvec3(2.0, 2.0, 0.0),
          glm::dvec3(-1.0 / glm::sqrt(2.0), -1.0 / glm::sqrt(2.0), 0.0)),
      cube,
      true,
      true,
      glm::dvec3(1.0, 1.0, 0.0));

  // works with a translated/rotated gltf
  checkIntersection(
      Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
      translatedCube,
      false,
      true,
      glm::dvec3(10.0, 10.0, 10.0 + 2.0 / glm::sqrt(2)));

  // avoids backface triangles
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 0.5), glm::dvec3(0.0, 0.0, -1.0)),
      cube,
      true,
      false,
      {});

  // hits backface triangles
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 0.5), glm::dvec3(0.0, 0.0, -1.0)),
      cube,
      false,
      true,
      glm::dvec3(0.0, 0.0, -1.0));
}
