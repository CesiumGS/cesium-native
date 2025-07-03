/* <editor-fold desc="MIT License">

Copyright(c) 2023 Timothy Moore

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

</editor-fold> */

#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumCurl/Library.h>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace CesiumCurl {

/**
 * @brief Options for constructing a \ref CurlAssetAccessor.
 */
struct CESIUMCURL_API CurlAssetAccessorOptions {
  /**
   * @brief The `User-Agent` header to include with each request.
   */
  std::string userAgent{"Mozilla/5.0 Cesium Native CurlAssetAccessor"};

  /**
   * @brief Request headers to automatically include in each request.
   *
   * Request headers passed to \ref CurlAssetAccessor::get or \ref
   * CurlAssetAccessor::request take precendence over these.
   */
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders{};

  /**
   * @brief Whether a PUT or POST to a `file:` URL is allowed to create file
   * system directories to hold the target file.
   *
   * This property has no effect when targetting a version of iOS prior to 13.
   */
  bool allowDirectoryCreation{false};

  /**
   * @brief The path to TLS certificates. If non-empty, this will be provided to
   * libcurl as `CURLOPT_CAPATH`.
   */
  std::string certificatePath{};

  /**
   * @brief A file containing TLS certificates. If non-empty, this will be
   * provided to libcurl as `CURLOPT_CAINFO`.
   */
  std::string certificateFile{};

  /**
   * @brief Whether to call `curl_global_init(CURL_GLOBAL_ALL)` at construction
   * time and `curl_global_cleanup()` at destruction time. Only set this to
   * false if the initialization and cleanup are done elsewhere.
   */
  bool doGlobalInit{true};
};

/**
 * @brief An implementation of `IAssetAccessor` that can make network and local
 * requests to a variety of servers using libcurl.
 */
class CESIUMCURL_API CurlAssetAccessor
    : public std::enable_shared_from_this<CurlAssetAccessor>,
      public CesiumAsync::IAssetAccessor {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param options The options with which to construct this instance.
   */
  CurlAssetAccessor(const CurlAssetAccessorOptions& options = {});
  ~CurlAssetAccessor() override;

  /**
   * @brief Gets the options that were used to construct this accessor.
   */
  const CurlAssetAccessorOptions& getOptions() const;

  /** @inheritdoc */
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers)
      override;

  /** @inheritdoc */
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
      const std::span<const std::byte>& contentPayload) override;

  /** @inheritdoc */
  void tick() noexcept override;

private:
  struct CurlCache;
  class CurlHandle;

  std::unique_ptr<CurlCache> _pCurlCache;
  CurlAssetAccessorOptions _options;
};

} // namespace CesiumCurl
