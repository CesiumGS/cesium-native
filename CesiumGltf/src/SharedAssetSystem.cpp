#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/SharedAssetSystem.h>

namespace CesiumGltf {

SharedAssetSystem::SharedAssetSystem() noexcept
    : _pImages(new CesiumAsync::SharedAssetDepot<CesiumGltf::ImageCesium>()) {}

SharedAssetSystem::~SharedAssetSystem() noexcept = default;

const CesiumAsync::SharedAssetDepot<CesiumGltf::ImageCesium>&
SharedAssetSystem::image() {
  return *this->_pImages;
}

} // namespace CesiumGltf
