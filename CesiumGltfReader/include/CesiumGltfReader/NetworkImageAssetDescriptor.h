#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <memory>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumGltfReader {

/**
 * @brief A description of an image asset that can be loaded from the network
 * using an {@link CesiumAsync::IAssetAccessor}. This includes a URL, any headers to be
 * included in the request, and the set of supported GPU texture formats for
 * KTX2 decoding.
 */
struct NetworkImageAssetDescriptor
    : public CesiumAsync::NetworkAssetDescriptor {
  /**
   * @brief The supported GPU texture formats used for KTX2 decoding.
   */
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets{};

  /**
   * @brief Determines if this descriptor is identical to another one.
   */
  bool operator==(const NetworkImageAssetDescriptor& rhs) const noexcept;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor and return the loaded {@link CesiumGltf::ImageAsset}.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the image asset once the request is
   * complete.
   */
  CesiumAsync::Future<CesiumUtility::ResultPointer<CesiumGltf::ImageAsset>>
  load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const;
};

} // namespace CesiumGltfReader

/** @brief Hash implementation for \ref
 * CesiumGltfReader::NetworkImageAssetDescriptor. */
template <> struct std::hash<CesiumGltfReader::NetworkImageAssetDescriptor> {
  /** @brief Returns a `size_t` hash of the provided \ref
   * CesiumGltfReader::NetworkImageAssetDescriptor. */
  std::size_t operator()(
      const CesiumGltfReader::NetworkImageAssetDescriptor& key) const noexcept;
};
