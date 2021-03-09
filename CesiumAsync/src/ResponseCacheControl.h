#pragma once

#include "CesiumAsync/HttpHeaders.h"
#include "CesiumAsync/Library.h"
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
      int maxAge,
      int sharedMaxAge);

  /**
   * @brief Must-Revalidate directive that appears in the Cache-Control header.
   */
  inline bool mustRevalidate() const { return _mustRevalidate; }

  /**
   * @brief No-Cache directive that appears in the Cache-Control header.
   */
  inline bool noCache() const { return _noCache; }

  /**
   * @brief No-Store directive that appears in the Cache-Control header.
   */
  inline bool noStore() const { return _noStore; }

  /**
   * @brief No-Transform directive that appears in the Cache-Control header.
   */
  inline bool noTransform() const { return _noTransform; }

  /**
   * @brief Public directive that appears in the Cache-Control header.
   */
  inline bool accessControlPublic() const { return _accessControlPublic; }

  /**
   * @brief Private directive that appears in the Cache-Control header.
   */
  inline bool accessControlPrivate() const { return _accessControlPrivate; }

  /**
   * @brief Proxy-Revalidate directive that appears in the Cache-Control header.
   */
  inline bool proxyRevalidate() const { return _proxyRevalidate; }

  /**
   * @brief Max-Age directive that appears in the Cache-Control header.
   */
  int maxAge() const { return _maxAge; }

  /**
   * @brief S-Maxage directive that appears in the Cache-Control header.
   */
  int sharedMaxAge() const { return _sharedMaxAge; }

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
  int _maxAge;
  int _sharedMaxAge;
};
} // namespace CesiumAsync
