#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace CesiumAsync {

class AsyncSystem;

/**
 * @brief A description of an asset that can be loaded from the network using an
 * {@link IAssetAccessor}. This includes a URL and any headers to be included
 * in the request.
 */
struct NetworkAssetDescriptor {
  /**
   * @brief The URL from which this network asset is downloaded.
   */
  std::string url;

  /**
   * @brief The HTTP headers used in requesting this asset.
   */
  std::vector<IAssetAccessor::THeader> headers;

  /**
   * @brief Determines if this descriptor is identical to another one.
   */
  bool operator==(const NetworkAssetDescriptor& rhs) const noexcept;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the request once it is complete.
   */
  Future<std::shared_ptr<CesiumAsync::IAssetRequest>> loadFromNetwork(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor and return the downloaded bytes.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the downloaded bytes once the request is
   * complete.
   */
  Future<CesiumUtility::Result<std::vector<std::byte>>> loadBytesFromNetwork(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const;
};

} // namespace CesiumAsync

/** @brief Hash implementation for \ref CesiumAsync::NetworkAssetDescriptor. */
template <> struct std::hash<CesiumAsync::NetworkAssetDescriptor> {
  /** @brief Returns a `size_t` hash of the provided \ref
   * CesiumAsync::NetworkAssetDescriptor. */
  std::size_t
  operator()(const CesiumAsync::NetworkAssetDescriptor& key) const noexcept;
};
