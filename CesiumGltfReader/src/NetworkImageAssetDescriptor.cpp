#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumGltfReader/NetworkImageAssetDescriptor.h>
#include <CesiumUtility/Hash.h>
#include <CesiumUtility/Result.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace CesiumGltfReader {

bool NetworkImageAssetDescriptor::operator==(
    const NetworkImageAssetDescriptor& rhs) const noexcept {
  if (!NetworkAssetDescriptor::operator==(rhs))
    return false;

  return this->ktx2TranscodeTargets.ETC1S_R ==
             rhs.ktx2TranscodeTargets.ETC1S_R &&
         this->ktx2TranscodeTargets.ETC1S_RG ==
             rhs.ktx2TranscodeTargets.ETC1S_RG &&
         this->ktx2TranscodeTargets.ETC1S_RGB ==
             rhs.ktx2TranscodeTargets.ETC1S_RGB &&
         this->ktx2TranscodeTargets.ETC1S_RGBA ==
             rhs.ktx2TranscodeTargets.ETC1S_RGBA &&
         this->ktx2TranscodeTargets.UASTC_R ==
             rhs.ktx2TranscodeTargets.UASTC_R &&
         this->ktx2TranscodeTargets.UASTC_RG ==
             rhs.ktx2TranscodeTargets.UASTC_RG &&
         this->ktx2TranscodeTargets.UASTC_RGB ==
             rhs.ktx2TranscodeTargets.UASTC_RGB &&
         this->ktx2TranscodeTargets.UASTC_RGBA ==
             rhs.ktx2TranscodeTargets.UASTC_RGBA;
}

Future<ResultPointer<ImageAsset>> NetworkImageAssetDescriptor::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return this->loadBytesFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread([ktx2TranscodeTargets = this->ktx2TranscodeTargets](
                              Result<std::vector<std::byte>>&& result) {
        if (!result.value) {
          return ResultPointer<ImageAsset>(result.errors);
        }

        ImageReaderResult imageResult =
            ImageDecoder::readImage(*result.value, ktx2TranscodeTargets);

        result.errors.merge(
            ErrorList{imageResult.errors, imageResult.warnings});

        return ResultPointer<ImageAsset>(
            imageResult.pImage,
            std::move(result.errors));
      });
}

} // namespace CesiumGltfReader

std::size_t std::hash<NetworkImageAssetDescriptor>::operator()(
    const NetworkImageAssetDescriptor& key) const noexcept {
  std::hash<NetworkAssetDescriptor> baseHash{};
  std::hash<GpuCompressedPixelFormat> ktxHash{};

  size_t result = baseHash(key);
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
