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
  std::optional<glm::dvec3> intersectionPoint =
      GltfUtilities::intersectRayGltfModel(
          Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
          cube);
  CHECK(intersectionPoint == glm::dvec3(0.0, 0.0, 1.0));

  // intersects a corner of the cube
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(2.0, 2.0, 0.0),
          glm::dvec3(-1.0 / glm::sqrt(2.0), -1.0 / glm::sqrt(2.0), 0.0)),
      cube);
  CHECK(glm::all(glm::lessThan(
      glm::abs(
          *intersectionPoint -
          glm::dvec3(1.0, 1.0, 0.0)),
      glm::dvec3(CesiumUtility::Math::Epsilon6))));

  // works with a translated/rotated gltf
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
      translatedCube,
      false);
  CHECK(glm::all(glm::lessThan(
      glm::abs(
          *intersectionPoint -
          glm::dvec3(10.0, 10.0, 10.0 + 2.0 / glm::sqrt(2))),
      glm::dvec3(CesiumUtility::Math::Epsilon6))));

  // avoids backface triangles
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(0.0, 0.0, 0.5), glm::dvec3(0.0, 0.0, -1.0)),
      cube);
  CHECK(intersectionPoint == std::nullopt);

  // hits backface triangles
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(0.0, 0.0, 0.5), glm::dvec3(0.0, 0.0, -1.0)),
      cube,
      false);
  CHECK(intersectionPoint == glm::dvec3(0.0, 0.0, -1.0));

  // intersects with top of sphere
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      sphere);
  CHECK(intersectionPoint.has_value());
  CHECK(glm::epsilonEqual((*intersectionPoint)[2], 1.0, Math::Epsilon6));

  // check that ray intersects approximately with sloped part of sphere
  intersectionPoint = GltfUtilities::intersectRayGltfModel(
      Ray(glm::dvec3(0.5, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
      sphere);
  CHECK(intersectionPoint.has_value());
  CHECK(glm::epsilonEqual(
      (*intersectionPoint)[2],
      glm::sqrt(0.75),
      Math::Epsilon2));
}
