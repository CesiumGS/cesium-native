#include <CesiumI3SSelection/SlpkResourceLocator.h>

#include <algorithm>
#include <string>

namespace CesiumI3SSelection {

namespace {
std::string normalizeSlashes(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  return path;
}
} // namespace

SlpkResourceLocator::SlpkResourceLocator(
    std::string slpkFilePath,
    std::string sublayerPrefix,
    std::string geometryExtension)
    : _slpkFilePath(normalizeSlashes(std::move(slpkFilePath))),
      _sublayerPrefix(std::move(sublayerPrefix)),
      _geometryExtension(std::move(geometryExtension)) {}

std::string
SlpkResourceLocator::makeEntryUrl(const std::string& entryPath) const {
  return "slpk:///" + this->_slpkFilePath + "!" + entryPath;
}

std::string SlpkResourceLocator::getLayerUrl() const {
  return this->makeEntryUrl("3DSceneLayer.json.gz");
}

std::string SlpkResourceLocator::getNodePageUrl(uint32_t pageIndex) const {
  return this->makeEntryUrl(
      this->_sublayerPrefix + "nodepages/" + std::to_string(pageIndex) +
      ".json.gz");
}

std::string SlpkResourceLocator::getGeometryUrl(
    uint32_t resourceId,
    uint32_t bufferIndex) const {
  return this->makeEntryUrl(
      this->_sublayerPrefix + "nodes/" + std::to_string(resourceId) +
      "/geometries/" + std::to_string(bufferIndex) + this->_geometryExtension);
}

} // namespace CesiumI3SSelection
