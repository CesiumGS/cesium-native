#include <CesiumI3SSelection/HttpResourceLocator.h>

#include <string>

namespace CesiumI3SSelection {

HttpResourceLocator::HttpResourceLocator(std::string baseUrl)
    : _baseUrl(std::move(baseUrl)) {}

std::string HttpResourceLocator::getLayerUrl() const { return this->_baseUrl; }

std::string HttpResourceLocator::getNodePageUrl(uint32_t pageIndex) const {
  return this->_baseUrl + "/nodepages/" + std::to_string(pageIndex) + "/";
}

std::string HttpResourceLocator::getGeometryUrl(
    uint32_t resourceId,
    uint32_t bufferIndex) const {
  return this->_baseUrl + "/nodes/" + std::to_string(resourceId) +
         "/geometries/" + std::to_string(bufferIndex) + "/";
}

} // namespace CesiumI3SSelection
