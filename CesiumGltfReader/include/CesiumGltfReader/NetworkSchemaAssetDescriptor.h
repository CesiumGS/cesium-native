#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <memory>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumGltfReader {

/**
 * @brief A description of a schema asset that can be loaded from the network
 * using an {@link CesiumAsync::IAssetAccessor}. This includes a URL and any headers to be
 * included in the request.
 */
struct NetworkSchemaAssetDescriptor
    : public CesiumAsync::NetworkAssetDescriptor {
  /**
   * @brief Determines if this descriptor is identical to another one.
   */
  bool operator==(const NetworkSchemaAssetDescriptor& rhs) const noexcept;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor and return the loaded {@link CesiumGltf::Schema}.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the schema asset once the request is
   * complete.
   */
  CesiumAsync::Future<CesiumUtility::ResultPointer<CesiumGltf::Schema>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const;
};

} // namespace CesiumGltfReader

/** @brief Hash implementation for \ref
 * CesiumGltfReader::NetworkSchemaAssetDescriptor. */
template <> struct std::hash<CesiumGltfReader::NetworkSchemaAssetDescriptor> {
  /** @brief Returns a `size_t` hash of the provided \ref
   * CesiumGltfReader::NetworkSchemaAssetDescriptor. */
  std::size_t operator()(
      const CesiumGltfReader::NetworkSchemaAssetDescriptor& key) const noexcept;
};
