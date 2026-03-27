#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumVectorData/GltfConverter.h>

#include <algorithm>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumVectorData;

namespace CesiumVectorData {

// This finds the average of geographic coordinates, but it's not a great way to
// find the actual average coordinate of the data.

glm::dvec3 computeCentroid(const std::vector<glm::dvec3>& positions) {
    glm::dvec3 centroid(0.0);
    for (const auto& position : positions) {
        centroid += position;
    }
    centroid /= static_cast<double>(positions.size());
    return centroid;
}

void transformIntoFrame(
    const glm::dmat4& localFrame,
    const std::vector<glm::dvec3>& positions,
    std::vector<glm::dvec3>& outPositions) {
  glm::dmat4 toLocal = inverse(localFrame);
  outPositions.resize(positions.size());
  std::transform(
      positions.begin(),
      positions.end(),
      outPositions.begin(),
      [&toLocal](const glm::dvec3& cartoDegrees) {
        CesiumGeospatial::Cartographic cartographic(
            Cartographic::fromDegrees(cartoDegrees.x, cartoDegrees.y, cartoDegrees.z));
        auto cartesian = CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(cartographic);
        return glm::dvec3(toLocal * glm::dvec4(cartesian, 1.0));
      });
}

ConverterResult GltfConverter::operator()(const GeoJsonDocument& geoJson) {
  ConverterResult result;
  const GeoJsonObject& root = geoJson.rootObject;
  // Look for linestrings, count objects and coordinates
  size_t numLineStrings = 0;
  size_t numCoordinates = 0;
  auto lineStringItr = root.allOfType<GeoJsonLineString>().begin();
  while (lineStringItr != root.allOfType<GeoJsonLineString>().end()) {
    numLineStrings++;
    numCoordinates += lineStringItr->coordinates.size();
  }
  std::vector<glm::dvec3> cartoCoordinates;
  cartoCoordinates.reserve(numCoordinates);
  lineStringItr = root.allOfType<GeoJsonLineString>().begin();
  while (lineStringItr != root.allOfType<GeoJsonLineString>().end()) {
    cartoCoordinates.insert(cartoCoordinates.end(), lineStringItr->coordinates.begin(), lineStringItr->coordinates.end());
  }
  glm::dvec3 centroid = computeCentroid(cartoCoordinates);
  //  local to global cartesian
  glm::dmat4 enuToFixedFrame = GlobeTransforms::eastNorthUpToFixedFrame(
      Ellipsoid::WGS84.cartographicToCartesian(Cartographic::fromDegrees(centroid.x, centroid.y, centroid.z)));
  std::vector<glm::dvec3> localPositions(numCoordinates);
  transformIntoFrame(enuToFixedFrame, cartoCoordinates, localPositions);
  return result;
}

}
