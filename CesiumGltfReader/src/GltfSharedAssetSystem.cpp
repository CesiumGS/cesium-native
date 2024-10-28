#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumGltfReader/ImageReader.h>
#include <CesiumUtility/Hash.h>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

std::size_t std::hash<NetworkImageAssetKey>::operator()(
    const NetworkImageAssetKey& key) const noexcept {
  std::hash<NetworkAssetKey> baseHash{};
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

namespace {

CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> createDefault() {
  CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> p =
      new GltfSharedAssetSystem();

  p->pImage.emplace(std::function(
      [](const AsyncSystem& asyncSystem,
         const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
         const NetworkImageAssetKey& key)
          -> Future<Result<IntrusivePointer<ImageAsset>>> {
        return key.load(asyncSystem, pAssetAccessor);
      }));

  return p;
}

} // namespace

namespace CesiumGltfReader {

Future<Result<IntrusivePointer<ImageAsset>>> NetworkImageAssetKey::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return this->loadBytesFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread([ktx2TranscodeTargets = this->ktx2TranscodeTargets](
                              Result<std::vector<std::byte>>&& result) {
        if (!result.value) {
          return Result<IntrusivePointer<ImageAsset>>(result.errors);
        }

        ImageReaderResult imageResult =
            ImageDecoder::readImage(*result.value, ktx2TranscodeTargets);

        result.errors.merge(
            ErrorList{imageResult.errors, imageResult.warnings});

        return Result<IntrusivePointer<ImageAsset>>(
            imageResult.pImage,
            std::move(result.errors));
      });
}

/*static*/ CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
GltfSharedAssetSystem::getDefault() {
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem> pDefault =
      createDefault();
  return pDefault;
}

} // namespace CesiumGltfReader
