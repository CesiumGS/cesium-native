#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/SharedAssetSystem.h>

namespace CesiumGltf {

SharedAssetSystem::SharedAssetSystem() noexcept
    : _pImages(new SharedAssetDepot<CesiumGltf::ImageCesium>()) {}

SharedAssetSystem::~SharedAssetSystem() noexcept = default;

const SharedAssetDepot<CesiumGltf::ImageCesium>& SharedAssetSystem::image() {
  return *this->_pImages;
}

} // namespace CesiumGltf
