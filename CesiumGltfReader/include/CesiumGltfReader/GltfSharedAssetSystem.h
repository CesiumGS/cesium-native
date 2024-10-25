#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>

namespace CesiumGltf {
struct ImageAsset;
}

namespace CesiumGltfReader {

class AssetSystemOptions {
public:
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets;
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;
};

struct NetworkImageAssetKey : public CesiumAsync::NetworkAssetKey {
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets{};
};

using NetworkImageAssetDepot =
    CesiumAsync::SharedAssetDepot<CesiumGltf::ImageAsset, NetworkImageAssetKey>;

/**
 * @brief Contains assets that are potentially shared across multiple glTF
 * models.
 */
class GltfSharedAssetSystem
    : public CesiumUtility::ReferenceCountedThreadSafe<GltfSharedAssetSystem> {
public:
  static CesiumUtility::IntrusivePointer<GltfSharedAssetSystem>
  getDefault(const AssetSystemOptions& options);

  CesiumUtility::IntrusivePointer<NetworkImageAssetDepot> pImage;
};

} // namespace CesiumGltfReader

template <> struct std::hash<CesiumGltfReader::NetworkImageAssetKey> {
  std::size_t
  operator()(const CesiumGltfReader::NetworkImageAssetKey& key) const noexcept;
};
