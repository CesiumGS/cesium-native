#pragma once

#include <CesiumAsync/SharedFuture.h>
#include <CesiumGltf/ImageCesium.h>

namespace CesiumGltf {

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class SharedAssetDepots
    : public CesiumUtility::ReferenceCountedThreadSafe<SharedAssetDepots> {
public:
  SharedAssetDepots() = default;
  void operator=(const SharedAssetDepots& other) = delete;

  /**
   * Obtains an existing {@link ImageCesium} or constructs a new one using the provided factory.
   */
  template <typename Factory>
  CesiumAsync::SharedFuture<
      CesiumUtility::IntrusivePointer<CesiumGltf::ImageCesium>>
  getOrFetch(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const Factory& factory,
      const std::string& uri,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
    return images
        .getOrFetch(asyncSystem, pAssetAccessor, factory, uri, headers);
  }

  const SharedAssetDepot<CesiumGltf::ImageCesium>* getImageDepot() {
    return &this->images;
  }

  void deletionTick() { this->images.deletionTick(); }

private:
  SharedAssetDepot<CesiumGltf::ImageCesium> images;
};

} // namespace CesiumGltf
