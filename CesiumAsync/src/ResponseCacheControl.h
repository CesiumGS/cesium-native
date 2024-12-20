#pragma once

#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/Library.h>
#include <CesiumUtility/Assert.h>

#include <map>
#include <optional>
#include <string>

namespace CesiumAsync {
/**
 * @brief Response cache control directives parsed from the response header.
 */
class ResponseCacheControl {
public:
  /**
   * @brief Constructor.
   * @param mustRevalidate Must-Revalidate directive that appears in the
   * Cache-Control header
   * @param noCache No-Cache directive that appears in the Cache-Control header
   * @param noStore No-Store directive that appears in the Cache-Control header
   * @param noTransform No-Transform directive that appears in the Cache-Control
   * header
   * @param accessControlPublic Public directive that appears in the
   * Cache-Control header
   * @param accessControlPrivate Private directive that appears in the
   * Cache-Control header
   * @param proxyRevalidate Proxy-Revalidate directive that appears in the
   * Cache-Control header
   * @param maxAge Max-Age directive that appears in the Cache-Control header
   * @param sharedMaxAge S-Maxage directive that appears in the Cache-Control
   * header
   */
  ResponseCacheControl(
      bool mustRevalidate,
      bool noCache,
      bool noStore,
      bool noTransform,
      bool accessControlPublic,
      bool accessControlPrivate,
      bool proxyRevalidate,
      std::optional<int> maxAge,
      std::optional<int> sharedMaxAge,
      std::optional<int> staleWhileRevalidate);

  /**
   * @brief Must-Revalidate directive that appears in the Cache-Control header.
   */
  inline bool mustRevalidate() const noexcept { return _mustRevalidate; }

  /**
   * @brief No-Cache directive that appears in the Cache-Control header.
   */
  inline bool noCache() const noexcept { return _noCache; }

  /**
   * @brief No-Store directive that appears in the Cache-Control header.
   */
  inline bool noStore() const noexcept { return _noStore; }

  /**
   * @brief No-Transform directive that appears in the Cache-Control header.
   */
  inline bool noTransform() const noexcept { return _noTransform; }

  /**
   * @brief Public directive that appears in the Cache-Control header.
   */
  inline bool accessControlPublic() const noexcept {
    return _accessControlPublic;
  }

  /**
   * @brief Private directive that appears in the Cache-Control header.
   */
  inline bool accessControlPrivate() const noexcept {
    return _accessControlPrivate;
  }

  /**
   * @brief Proxy-Revalidate directive that appears in the Cache-Control header.
   */
  inline bool proxyRevalidate() const noexcept { return _proxyRevalidate; }

  /**
   * @brief Existence of Max-Age directive that appears in the Cache-Control
   * header.
   */
  bool maxAgeExists() const noexcept { return _maxAge.has_value(); }

  /**
   * @brief Value of Max-Age directive that appears in the Cache-Control header.
   */
  int maxAgeValue() const noexcept {
    CESIUM_ASSERT(_maxAge.has_value());
    return *_maxAge;
  }

  /**
   * @brief Existence of S-Maxage directive that appears in the Cache-Control
   * header.
   */
  bool sharedMaxAgeExists() const noexcept { return _sharedMaxAge.has_value(); }

  /**
   * @brief Value of S-Maxage directive that appears in the Cache-Control
   * header.
   */
  int sharedMaxAgeValue() const noexcept {
    CESIUM_ASSERT(_sharedMaxAge.has_value());
    return *_sharedMaxAge;
  }

  /**
   * @brief Existence of Stale-While-Revalidate directive that appears in the
   * Cache-Control header.
   */
  bool staleWhileRevalidateExists() const noexcept {
    return _staleWhileRevalidate.has_value();
  }

  /**
   * @brief Value of Stale-While-Revalidate directive that appears in the
   * Cache-Control header.
   */
  int staleWhileRevalidateValue() const noexcept {
    CESIUM_ASSERT(_staleWhileRevalidate.has_value());
    return *_staleWhileRevalidate;
  }

  /**
   * @brief Parse response cache control from the response's headers.
   */
  static std::optional<ResponseCacheControl>
  parseFromResponseHeaders(const HttpHeaders& headers);

private:
  bool _mustRevalidate;
  bool _noCache;
  bool _noStore;
  bool _noTransform;
  bool _accessControlPublic;
  bool _accessControlPrivate;
  bool _proxyRevalidate;

  std::optional<int> _maxAge;
  std::optional<int> _sharedMaxAge;
  std::optional<int> _staleWhileRevalidate;
};
} // namespace CesiumAsync
