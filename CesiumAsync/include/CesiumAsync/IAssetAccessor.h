#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/Library.h>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace CesiumAsync {

class AsyncSystem;

/**
 * @brief Provides asynchronous access to assets, usually files downloaded via
 * HTTP.
 */
class CESIUMASYNC_API IAssetAccessor {
public:
  /**
   * @brief An HTTP header represented as a key/value pair.
   */
  typedef std::pair<std::string, std::string> THeader;

  virtual ~IAssetAccessor() = default;

  /**
   * @brief Starts a new request for the asset with the given URL.
   * The request proceeds asynchronously without blocking the calling thread.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param url The URL of the asset.
   * @param headers The headers to include in the request.
   * @return The in-progress asset request.
   */
  virtual CesiumAsync::Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) = 0;

  /**
   * @brief Starts a new request to the given URL, using the provided HTTP verb
   * and the provided content payload.
   *
   * The request proceeds asynchronously without blocking the calling thread.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param verb The HTTP verb to use, such as "POST" or "PATCH".
   * @param url The URL of the asset.
   * @param headers The headers to include in the request.
   * @param contentPayload The payload data to include in the request.
   * @return The in-progress asset request.
   */
  virtual CesiumAsync::Future<std::shared_ptr<IAssetRequest>> request(
      const AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const std::span<const std::byte>& contentPayload = {}) = 0;

  /**
   * @brief Ticks the asset accessor system while the main thread is blocked.
   *
   * If the asset accessor is not dependent on the main thread to
   * dispatch requests, this method does not need to do anything.
   */
  virtual void tick() noexcept = 0;
};

} // namespace CesiumAsync
