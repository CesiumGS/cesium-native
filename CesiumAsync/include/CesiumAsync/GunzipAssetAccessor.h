#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

namespace CesiumAsync {
class AsyncSystem;

/**
 * @brief A decorator for an {@link IAssetAccessor} that automatically unzips
 * gzipped asset responses from the underlying Asset Accessor.
 */
class GunzipAssetAccessor : public IAssetAccessor {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param pAssetAccessor The underlying {@link IAssetAccessor} used to
   * retrieve assets that may or may not be zipped.
   */
  GunzipAssetAccessor(const std::shared_ptr<IAssetAccessor>& pAssetAccessor);

  virtual ~GunzipAssetAccessor() noexcept override;

  /** @copydoc IAssetAccessor::get */
  virtual Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers) override;

  virtual Future<std::shared_ptr<IAssetRequest>> request(
      const AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers,
      const std::span<const std::byte>& contentPayload) override;

  /** @copydoc IAssetAccessor::tick */
  virtual void tick() noexcept override;

private:
  std::shared_ptr<IAssetAccessor> _pAssetAccessor;
};
} // namespace CesiumAsync
