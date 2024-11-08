#include "CesiumRasterOverlays/NetworkRasterOverlayImageAssetDescriptor.h"

#include "CesiumGltf/Ktx2TranscodeTargets.h"
#include "CesiumGltfReader/ImageDecoder.h"
#include "CesiumUtility/Hash.h"

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

std::size_t std::hash<NetworkRasterOverlayImageAssetDescriptor>::operator()(
    const NetworkRasterOverlayImageAssetDescriptor& key) const noexcept {
  std::hash<CesiumAsync::NetworkAssetDescriptor> baseHash{};
  std::hash<CesiumRasterOverlays::LoadTileImageFromUrlOptions> optionsHash{};
  std::hash<CesiumGltf::GpuCompressedPixelFormat> ktxHash{};

  size_t result = baseHash(key);
  result = Hash::combine(result, optionsHash(key.loadTileOptions));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.ETC1S_R));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.ETC1S_RG));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.ETC1S_RGB));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.ETC1S_RGBA));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.UASTC_R));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.UASTC_RG));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.UASTC_RGB));
  result = Hash::combine(result, ktxHash(key.ktx2TranscodeTargets.UASTC_RGBA));
  return result;
}

std::size_t
std::hash<CesiumRasterOverlays::LoadTileImageFromUrlOptions>::operator()(
    const CesiumRasterOverlays::LoadTileImageFromUrlOptions& key)
    const noexcept {
  std::hash<CesiumGeometry::Rectangle> rectHash{};
  std::hash<Credit> creditHash{};
  std::hash<bool> flagHash{};

  size_t result = rectHash(key.rectangle);
  for (const Credit& credit : key.credits) {
    result = Hash::combine(result, creditHash(credit));
  }

  result = Hash::combine(result, flagHash(key.moreDetailAvailable));
  result = Hash::combine(result, flagHash(key.allowEmptyImages));
  return result;
}

bool CesiumRasterOverlays::NetworkRasterOverlayImageAssetDescriptor::operator==(
    const NetworkRasterOverlayImageAssetDescriptor& rhs) const noexcept {
  return this->loadTileOptions == rhs.loadTileOptions &&
         NetworkAssetDescriptor::operator==(rhs);
}

CesiumAsync::Future<CesiumUtility::ResultPointer<LoadedRasterOverlayImage>>
CesiumRasterOverlays::NetworkRasterOverlayImageAssetDescriptor::load(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const {
  return this->loadBytesFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread([url = this->url,
                           options = this->loadTileOptions,
                           ktx2TranscodeTargets = this->ktx2TranscodeTargets](
                              Result<std::vector<std::byte>>&& result) {
        CesiumUtility::IntrusivePointer<LoadedRasterOverlayImage> pLoadedImage;
        pLoadedImage.emplace();
        pLoadedImage->credits = std::move(options.credits);
        pLoadedImage->rectangle = options.rectangle;
        pLoadedImage->moreDetailAvailable = options.moreDetailAvailable;

        CESIUM_TRACE("load image");
        if (result.errors) {
          return ResultPointer<LoadedRasterOverlayImage>(
              pLoadedImage,
              std::move(result.errors));
        }

        if (!result.value || result.value->empty()) {
          if (options.allowEmptyImages) {
            pLoadedImage->pImage.emplace();
            return ResultPointer<LoadedRasterOverlayImage>(
                pLoadedImage,
                std::move(result.errors));
          }

          ErrorList errors;
          errors.emplaceError(
              fmt::format("Image response for {} is empty.", url));
          return ResultPointer<LoadedRasterOverlayImage>(
              pLoadedImage,
              std::move(result.errors));
        }

        CesiumGltfReader::ImageReaderResult loadedImage =
            CesiumGltfReader::ImageDecoder::readImage(
                *result.value,
                ktx2TranscodeTargets);

        if (!loadedImage.errors.empty()) {
          loadedImage.errors.push_back("Image url: " + url);
        }
        if (!loadedImage.warnings.empty()) {
          loadedImage.warnings.push_back("Image url: " + url);
        }

        pLoadedImage->pImage = std::move(loadedImage.pImage);

        return ResultPointer<LoadedRasterOverlayImage>(
            pLoadedImage,
            std::move(result.errors));
      });
}
