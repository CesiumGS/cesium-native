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

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace CesiumCurl {

/**
 * @brief An implementation of `IAssetAccessor` that can make network and local
 * requests to a variety of servers using libcurl.
 */
class UrlAssetAccessor final
    : public std::enable_shared_from_this<UrlAssetAccessor>,
      public CesiumAsync::IAssetAccessor {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param certificatePath The path to TLS certificates. If non-empty, this
   * will be provided to libcurl as `CURLOPT_CAPATH`.
   * @param certificateFile A file containing TLS certificates. If non-empty,
   * this will be provided to libcurl as `CURLOPT_CAINFO`.
   */
  UrlAssetAccessor(
      const std::filesystem::path& certificatePath = {},
      const std::filesystem::path& certificateFile = {});
  ~UrlAssetAccessor() override;

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers)
      override;

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
      const std::span<const std::byte>& contentPayload) override;

  void tick() noexcept override;

private:
  struct CurlCache;
  class CurlHandle;

  std::unique_ptr<CurlCache> _pCurlCache;
  std::string _userAgent;
  std::string _certificatePath;
  std::string _certificateFile;
};

} // namespace CesiumCurl
