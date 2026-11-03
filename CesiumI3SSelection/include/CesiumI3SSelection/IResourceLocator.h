#pragma once

#include <CesiumI3SSelection/Library.h>

#include <cstdint>
#include <string>

namespace CesiumI3SSelection {

/**
 * @brief Abstracts URL / path construction for I3S resources.
 *
 * Concrete implementations produce either HTTP URLs (for live services) or
 * SLPK entry paths (for local ZIP archives).  The actual network / file I/O
 * is performed by the IAssetAccessor; this interface is solely responsible
 * for building the address strings.
 */
class CESIUMI3SSELECTION_API IResourceLocator {
public:
  virtual ~IResourceLocator() = default;

  /** @brief Returns the URL / path for the scene-layer JSON. */
  virtual std::string getLayerUrl() const = 0;

  /**
   * @brief Returns the URL / path for a node-page.
   * @param pageIndex Zero-based page index.
   */
  virtual std::string getNodePageUrl(uint32_t pageIndex) const = 0;

  /**
   * @brief Returns the URL / path for a geometry buffer.
   * @param resourceId Node resource identifier.
   * @param bufferIndex Index of the geometry buffer within the definition.
   */
  virtual std::string
  getGeometryUrl(uint32_t resourceId, uint32_t bufferIndex) const = 0;
};

} // namespace CesiumI3SSelection
