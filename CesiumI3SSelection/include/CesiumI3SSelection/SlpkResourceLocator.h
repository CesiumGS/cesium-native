#pragma once

#include <CesiumI3SSelection/IResourceLocator.h>

#include <string>

namespace CesiumI3SSelection {

/**
 * @brief Builds entry-path URIs for resources inside an SLPK (ZIP) file.
 *
 * URIs use the scheme `slpk:///<slpkFilePath>!<entryPath>` and require an
 * SLPK-aware `IAssetAccessor` implementation to resolve them.
 *
 * Examples for a mesh SLPK at `C:/data/Rancho.slpk` (no sublayer prefix):
 *  - layer JSON  : `slpk:///C:/data/Rancho.slpk!3DSceneLayer.json.gz`
 *  - node page 0 : `slpk:///C:/data/Rancho.slpk!nodepages/0.json.gz`
 *  - geometry 5/0: `slpk:///C:/data/Rancho.slpk!nodes/5/geometries/0.bin.gz`
 *
 * For a building SLPK with sublayer prefix `sublayers/31/`:
 *  - node page 0 : `slpk:///...!sublayers/31/nodepages/0.json.gz`
 */
class CESIUMI3SSELECTION_API SlpkResourceLocator final
    : public IResourceLocator {
public:
  /**
   * @brief Constructs the locator.
   *
   * @param slpkFilePath Absolute path to the SLPK file (backslashes are
   *        normalised to forward slashes).
   * @param sublayerPrefix Optional prefix prepended to all resource entry paths
   *        (e.g. `"sublayers/31/"` for building scene layers that contain a
   *        single active sublayer).  Leave empty for 3D Object / mesh layers.
   * @param geometryExtension File extension for geometry entries (default
   *        `".bin.gz"`; use `".bin.pccxyz"` for point-cloud layers).
   */
  SlpkResourceLocator(
      std::string slpkFilePath,
      std::string sublayerPrefix = {},
      std::string geometryExtension = ".bin.gz");

  std::string getLayerUrl() const override;
  std::string getNodePageUrl(uint32_t pageIndex) const override;
  std::string
  getGeometryUrl(uint32_t resourceId, uint32_t bufferIndex) const override;

private:
  std::string _slpkFilePath;
  std::string _sublayerPrefix;
  std::string _geometryExtension;

  std::string makeEntryUrl(const std::string& entryPath) const;
};

} // namespace CesiumI3SSelection
