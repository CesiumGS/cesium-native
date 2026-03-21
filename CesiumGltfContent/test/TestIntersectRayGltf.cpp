#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/vector_relational.hpp>

#include <cstddef>
#include <filesystem>
#include <string>

using namespace CesiumUtility;
using namespace CesiumGltf;
using namespace CesiumGeometry;
using namespace CesiumGltfReader;
using namespace CesiumGltfContent;

void checkIntersection(
    const Ray& ray,
    const Model& model,
    bool cullBackFaces,
    const glm::dmat4x4& modelToWorld,
    bool shouldHit,
    const glm::dvec3& expectedHit) {
  GltfUtilities::IntersectResult hitResult =
      GltfUtilities::intersectRayGltfModel(
          ray,
          model,
          cullBackFaces,
          modelToWorld);

  if (shouldHit) {
    CHECK(hitResult.hit.has_value());
    if (!hitResult.hit.has_value())
      return;
  } else {
    CHECK(!hitResult.hit.has_value());
    return;
  }

  // Validate hit point
  CHECK(glm::all(glm::lessThan(
      glm::abs(hitResult.hit->worldPoint - expectedHit),
      glm::dvec3(CesiumUtility::Math::Epsilon6))));

  // Use results to dive into model
  CHECK(hitResult.hit->meshId > -1);
  CHECK(static_cast<size_t>(hitResult.hit->meshId) < model.meshes.size());
  const CesiumGltf::Mesh& mesh =
      model.meshes[static_cast<size_t>(hitResult.hit->meshId)];

  CHECK(hitResult.hit->primitiveId > -1);
  CHECK(
      static_cast<size_t>(hitResult.hit->primitiveId) < mesh.primitives.size());
  const CesiumGltf::MeshPrimitive& primitive =
      mesh.primitives[static_cast<size_t>(hitResult.hit->primitiveId)];

  bool modeIsValid =
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES ||
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP ||
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN;
  CHECK(modeIsValid);

  // There should be positions
  auto positionAccessorIt = primitive.attributes.find("POSITION");
  CHECK(positionAccessorIt != primitive.attributes.end());

  // And a way to access them
  int positionAccessorID = positionAccessorIt->second;
  const Accessor* pPositionAccessor =
      Model::getSafe(&model.accessors, positionAccessorID);
  CHECK(pPositionAccessor);
}

void checkBadUnitCube(const std::string& testModelName, bool shouldHitAnyway) {
  GltfReader reader;
  Model testModel =
      *reader
           .readGltf(readFile(
               std::filesystem::path(CesiumGltfContent_TEST_DATA_DIR) /
               testModelName))
           .model;

  // Do an intersection with top side of the cube
  GltfUtilities::IntersectResult hitResult =
      GltfUtilities::intersectRayGltfModel(
          Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
          testModel,
          true,
          glm::dmat4x4(1.0));

  // We're expecting a bad model, so it shouldn't crash or assert
  // and we should get some warnings about that
  CHECK(hitResult.warnings.size() > 0);

  // Check for a bad model that is mostly good, and should produce good results
  if (shouldHitAnyway) {
    CHECK(hitResult.hit.has_value());
  } else {
    CHECK(!hitResult.hit.has_value());
  }
}

void checkValidUnitCube(const std::string& testModelName) {
  GltfReader reader;
  Model testModel =
      *reader
           .readGltf(readFile(
               std::filesystem::path(CesiumGltfContent_TEST_DATA_DIR) /
               testModelName))
           .model;

  // intersects the top side of the cube
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      glm::dmat4x4(1.0),
      true,
      glm::dvec3(0.0, 0.0, 0.5));

  // misses the top side of the cube to the right
  checkIntersection(
      Ray(glm::dvec3(0.6, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      glm::dmat4x4(1.0),
      false,
      {});

  // misses the top side of the cube because it's behind it (avoid backfaces)
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      glm::dmat4x4(1.0),
      false,
      {});

  // hits backface triangles
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      false,
      glm::dmat4x4(1.0),
      true,
      glm::dvec3(0.0, 0.0, -0.5));

  // tests against backfaces, and picks first hit (top)
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      false,
      glm::dmat4x4(1.0),
      true,
      glm::dvec3(0.0, 0.0, 0.5));

  // tests against backfaces, and picks first hit (bottom)
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, 1.0)),
      testModel,
      false,
      glm::dmat4x4(1.0),
      true,
      glm::dvec3(0.0, 0.0, -0.5));

  // just misses the top side of a cube translated to the right
  glm::dmat4x4 translationMatrix(1.0);
  translationMatrix[3] = glm::dvec4(0.6, 0.0, 0.0, 1.0);
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      translationMatrix,
      false,
      {});

  // just hits the top side of a cube translated to the right
  checkIntersection(
      Ray(glm::dvec3(0.6, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      translationMatrix,
      true,
      glm::dvec3(0.6, 0.0, 0.5));

  // correctly hits a scaled cube
  glm::dmat4x4 scaleMatrix = glm::dmat4(
      glm::dvec4(2.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 2.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 2.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
  checkIntersection(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      scaleMatrix,
      true,
      glm::dvec3(0.0, 0.0, 1.0));

  // just misses a scaled cube to the right
  checkIntersection(
      Ray(glm::dvec3(1.1, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      testModel,
      true,
      scaleMatrix,
      false,
      {});
}

TEST_CASE("GltfUtilities::intersectRayGltfModel") {
  checkValidUnitCube("cube.glb");
  checkValidUnitCube("cubeIndexed.glb");
  checkValidUnitCube("cubeStrip.glb");
  checkValidUnitCube("cubeStripIndexed.glb");
  checkValidUnitCube("cubeFan.glb");
  checkValidUnitCube("cubeFanIndexed.glb");
  checkValidUnitCube("cubeQuantized.glb");
  checkValidUnitCube("cubeTranslated.glb");

  checkBadUnitCube("cubeInvalidVertCount.glb", false);
  checkBadUnitCube("cubeSomeBadIndices.glb", true);
}
