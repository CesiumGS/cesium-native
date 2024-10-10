#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumAsync/SharedFuture.h>

namespace CesiumGltf {

struct ImageCesium;

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class SharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<SharedAssetSystem> {
public:
  SharedAssetSystem() noexcept;
  ~SharedAssetSystem() noexcept;

  SharedAssetSystem(const SharedAssetSystem&) = delete;
  void operator=(const SharedAssetSystem& other) = delete;

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
    return this->_pImages
        ->getOrFetch(asyncSystem, pAssetAccessor, factory, uri, headers);
  }

  const CesiumAsync::SharedAssetDepot<CesiumGltf::ImageCesium>& image();

private:
  CesiumUtility::IntrusivePointer<
      CesiumAsync::SharedAssetDepot<CesiumGltf::ImageCesium>>
      _pImages;
};

} // namespace CesiumGltf
