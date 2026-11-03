#pragma once

#include <CesiumI3SSelection/IResourceLocator.h>

#include <string>

namespace CesiumI3SSelection {

/**
 * @brief Builds HTTP URLs for I3S resources on a live scene-service endpoint.
 *
 * All paths follow the standard I3S REST API layout:
 *  - layer JSON   : `{baseUrl}`
 *  - node page N  : `{baseUrl}/nodepages/{N}/`
 *  - geometry R/B : `{baseUrl}/nodes/{R}/geometries/{B}/`
 */
class CESIUMI3SSELECTION_API HttpResourceLocator final
    : public IResourceLocator {
public:
  /**
   * @brief Constructs the locator.
   * @param baseUrl Base URL of the I3S scene layer (the `layers/0` endpoint).
   */
  explicit HttpResourceLocator(std::string baseUrl);

  std::string getLayerUrl() const override;
  std::string getNodePageUrl(uint32_t pageIndex) const override;
  std::string
  getGeometryUrl(uint32_t resourceId, uint32_t bufferIndex) const override;

private:
  std::string _baseUrl;
};

} // namespace CesiumI3SSelection
