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

// Copyright 2024 CesiumGS, Inc. and Contributors

// The curl headers include Windows headers. Don't let Windows create `min` and
// `max` #defines.
#define NOMINMAX

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/CurlAssetAccessor.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/Uri.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/system.h>
#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumCurl {

const auto CURL_BUFFERSIZE = 3145728L; // 3 MiB

// A cache that permits reuse of CURL handles. This is extremely important for
// performance because libcurl will keep existing connections open if a curl
// handle is not destroyed ("cleaned up").

struct CurlAssetAccessor::CurlCache {
  struct CacheEntry {
    CacheEntry() : curl(nullptr), free(false) {}
    CacheEntry(CURL* curl_, bool free_) : curl(curl_), free(free_) {}
    CURL* curl;
    bool free;
  };
  std::mutex cacheMutex;
  std::vector<CacheEntry> cache;
  CURL* get() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    for (auto& entry : cache) {
      if (entry.free) {
        entry.free = false;
        return entry.curl;
      }
    }
    cache.emplace_back(curl_easy_init(), false);
    return cache.back().curl;
  }
  void release(CURL* curl) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    for (auto& entry : cache) {
      if (curl == entry.curl) {
        curl_easy_reset(curl);
        entry.free = true;
        return;
      }
    }
    throw std::logic_error("releasing a curl handle that is not in the cache");
  }

  friend class CurlHandle;
};

// RAII wrapper for the CurlCache.
class CurlAssetAccessor::CurlHandle {
public:
  CurlHandle(CurlAssetAccessor* accessor)
      : _accessor(accessor), _curl(accessor->_pCurlCache->get()) {}

  ~CurlHandle() {
    if (this->_accessor) {
      this->_accessor->_pCurlCache->release(this->_curl);
    }
  }

  CURL* operator()() const { return this->_curl; }

  CurlHandle& operator=(CurlHandle&& rhs) noexcept {
    std::swap(this->_accessor, rhs._accessor);
    std::swap(this->_curl, rhs._curl);
    return *this;
  }

  CurlHandle(CurlHandle&& rhs) noexcept
      : _accessor(rhs._accessor), _curl(rhs._curl) {
    rhs._accessor = nullptr;
    rhs._curl = nullptr;
  }

  CurlHandle& operator=(const CurlHandle& rhs) = delete;
  CurlHandle(const CurlHandle& rhs) = delete;

private:
  CurlAssetAccessor* _accessor;
  CURL* _curl;
};

namespace {

class CurlAssetResponse final : public IAssetResponse {
public:
  [[nodiscard]] uint16_t statusCode() const override { return _statusCode; }

  [[nodiscard]] std::string contentType() const override {
    return _contentType;
  }

  [[nodiscard]] const HttpHeaders& headers() const override { return _headers; }

  [[nodiscard]] std::span<const std::byte> data() const override {
    return {const_cast<const std::byte*>(_result.data()), _result.size()};
  }

  static size_t
  headerCallback(char* buffer, size_t size, size_t nitems, void* userData);
  static size_t
  dataCallback(char* buffer, size_t size, size_t nitems, void* userData);
  void setCallbacks(CURL* curl);
  uint16_t _statusCode = 0;
  std::string _contentType;
  HttpHeaders _headers;
  std::vector<std::byte> _result;
};

class CurlAssetRequest final : public IAssetRequest {
public:
  CurlAssetRequest(
      const std::string& method,
      const std::string& url,
      const HttpHeaders& headers,
      std::vector<std::byte>&& contentPayload)
      : _method(method),
        _url(url),
        _headers(headers),
        _contentPayload(std::move(contentPayload)),
        _bytesSent(0) {}

  CurlAssetRequest(
      const std::string& method,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& thisRequestHeaders,
      const std::vector<IAssetAccessor::THeader>& accessorRequestHeaders,
      std::vector<std::byte>&& contentPayload)
      : _method(method),
        _url(url),
        _contentPayload(std::move(contentPayload)),
        _bytesSent(0) {
    // Add the accessor headers second because we want this request's headers to
    // take precedence. `insert` will skip insertion if the key already exists.
    this->_headers.insert(thisRequestHeaders.begin(), thisRequestHeaders.end());
    this->_headers.insert(
        accessorRequestHeaders.begin(),
        accessorRequestHeaders.end());
  }

  [[nodiscard]] const std::string& method() const override {
    return this->_method;
  }

  [[nodiscard]] const std::string& url() const override { return this->_url; }

  [[nodiscard]] const HttpHeaders& headers() const override {
    return this->_headers;
  }

  [[nodiscard]] std::span<const std::byte> contentPayload() const {
    return this->_contentPayload;
  }

  [[nodiscard]] const IAssetResponse* response() const override {
    return this->_response.get();
  }

  void setResponse(std::unique_ptr<CurlAssetResponse> response) {
    this->_response = std::move(response);
  }

  static size_t uploadDataCallback(
      void* buffer,
      size_t size,
      size_t count,
      CurlAssetRequest* pRequest);

private:
  std::string _method;
  std::string _url;
  HttpHeaders _headers;
  std::unique_ptr<CurlAssetResponse> _response;
  std::vector<std::byte> _contentPayload;
  size_t _bytesSent;
};

/* static */ size_t CurlAssetRequest::uploadDataCallback(
    void* buffer,
    size_t size,
    size_t count,
    CurlAssetRequest* pRequest) {
  assert(pRequest);
  assert(buffer);
  size_t bytesRemaining =
      pRequest->_contentPayload.size() - pRequest->_bytesSent;
  size_t bufferSpace = size * count;
  size_t bytesToSend = std::min(bytesRemaining, bufferSpace);
  std::memcpy(
      buffer,
      pRequest->_contentPayload.data() + pRequest->_bytesSent,
      bytesToSend);
  pRequest->_bytesSent += bytesToSend;
  return bytesToSend;
}

size_t CurlAssetResponse::headerCallback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userData) {
  // size is supposed to always be 1, but who knows
  const size_t cnt = size * nitems;
  auto* response = static_cast<CurlAssetResponse*>(userData);
  if (!response) {
    return cnt;
  }
  auto* colon = static_cast<char*>(std::memchr(buffer, ':', nitems));
  if (colon) {
    char* value = colon + 1;
    auto* end = std::find(value, buffer + cnt, '\r');
    while (value < end && *value == ' ') {
      ++value;
    }
    response->_headers.insert(
        {std::string(buffer, colon), std::string(value, end)});
    auto contentTypeItr = response->_headers.find("content-type");
    if (contentTypeItr != response->_headers.end()) {
      response->_contentType = contentTypeItr->second;
    }
  }
  return cnt;
}

size_t CurlAssetResponse::dataCallback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userData) {
  const size_t cnt = size * nitems;
  auto* response = static_cast<CurlAssetResponse*>(userData);
  if (!response) {
    return cnt;
  }
  std::transform(
      buffer,
      buffer + cnt,
      std::back_inserter(response->_result),
      [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
  return cnt;
}

void CurlAssetResponse::setCallbacks(CURL* curl) {
  curl_easy_setopt(
      curl,
      CURLOPT_WRITEFUNCTION,
      CurlAssetResponse::dataCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(
      curl,
      CURLOPT_HEADERFUNCTION,
      CurlAssetResponse::headerCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
}

curl_slist* setCommonOptions(
    CURL* curl,
    const std::string& url,
    const HttpHeaders& headers,
    const std::string& userAgent,
    const std::string& certificatePath,
    const std::string& certificateFile) {
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (!certificateFile.empty()) {
    curl_easy_setopt(curl, CURLOPT_CAINFO, certificateFile.c_str());
  }
  if (!certificatePath.empty()) {
    curl_easy_setopt(curl, CURLOPT_CAPATH, certificatePath.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_BUFFERSIZE);
  curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 20L);
  curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 300L);
  // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_slist* list = nullptr;
  for (const auto& header : headers) {
    std::string fullHeader = header.first + ":" + header.second;
    list = curl_slist_append(list, fullHeader.c_str());
  }
  if (list) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  return list;
}

} // namespace

CurlAssetAccessor::CurlAssetAccessor(const CurlAssetAccessorOptions& options)
    : _pCurlCache(std::make_unique<CurlCache>()), _options(options) {
  if (this->_options.doGlobalInit) {
    curl_global_init(CURL_GLOBAL_ALL);
  }
}

CurlAssetAccessor::~CurlAssetAccessor() {
  if (this->_options.doGlobalInit) {
    curl_global_cleanup();
  }
}

const CurlAssetAccessorOptions& CurlAssetAccessor::getOptions() const {
  return this->_options;
}

Future<std::shared_ptr<IAssetRequest>> CurlAssetAccessor::get(
    const AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers) {
  return asyncSystem.runInWorkerThread(
      [url,
       headers,
       pThis = this->shared_from_this()]() -> std::shared_ptr<IAssetRequest> {
        CESIUM_TRACE("CurlAssetAccessor::get");
        std::shared_ptr<CurlAssetRequest> pRequest =
            std::make_shared<CurlAssetRequest>(
                "GET",
                url,
                headers,
                pThis->_options.requestHeaders,
                std::vector<std::byte>());

        CurlHandle curl(pThis.get());
        curl_slist* list = setCommonOptions(
            curl(),
            pRequest->url(),
            pRequest->headers(),
            pThis->_options.userAgent,
            pThis->_options.certificatePath,
            pThis->_options.certificateFile);
        std::unique_ptr<CurlAssetResponse> pResponse =
            std::make_unique<CurlAssetResponse>();
        pResponse->setCallbacks(curl());
        CURLcode responseCode = curl_easy_perform(curl());
        curl_slist_free_all(list);
        if (responseCode == 0) {
          // Use `long` instead of int64_t to match the documented
          // `CURLINFO_RESPONSE_CODE` type.
          // NOLINTNEXTLINE(google-runtime-int)
          long httpResponseCode = 0;
          curl_easy_getinfo(curl(), CURLINFO_RESPONSE_CODE, &httpResponseCode);
          pResponse->_statusCode = static_cast<uint16_t>(httpResponseCode);
          // The response header callback also sets _contentType, so not sure
          // that this is necessary...
          char* ct = nullptr;
          curl_easy_getinfo(curl(), CURLINFO_CONTENT_TYPE, &ct);
          if (ct) {
            pResponse->_contentType = ct;
          }
          pRequest->setResponse(std::move(pResponse));
          return pRequest;
        } else {
          throw std::runtime_error(fmt::format(
              "{} `{}` failed: {}",
              pRequest->method(),
              pRequest->url(),
              curl_easy_strerror(responseCode)));
        }
      });
}

#if !defined(TARGET_OS_IOS) || __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
namespace {
constexpr std::string_view fileScheme("file:");

bool isFile(const std::string& url) {
  return Uri(url).getScheme() == fileScheme;
}

std::string convertFileUriToFilename(const std::string& url) {
  return Uri::uriPathToNativePath(std::string(Uri(url).getPath()));
}
} // namespace
#endif

Future<std::shared_ptr<IAssetRequest>> CurlAssetAccessor::request(
    const AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const std::span<const std::byte>& contentPayload) {
  std::vector<std::byte> payloadCopy(
      contentPayload.begin(),
      contentPayload.end());
  return asyncSystem.runInWorkerThread(
      [url,
       headers,
       verb,
       payloadCopy = std::move(payloadCopy),
       pThis = this->shared_from_this()]() mutable
      -> std::shared_ptr<IAssetRequest> {
        CESIUM_TRACE("CurlAssetAccessor::request");

        CURLoption verbOption;
        if (verb == "POST") {
          verbOption = CURLOPT_POST;
        } else if (verb == "PUT") {
          verbOption = CURLOPT_UPLOAD;
        } else {
          throw std::runtime_error(fmt::format(
              "CurlAssetAccessor does not support verb `{}`.",
              verb));
        }

        auto pRequest = std::make_shared<CurlAssetRequest>(
            verb,
            url,
            headers,
            pThis->_options.requestHeaders,
            std::move(payloadCopy));

    // These APIs don't exist in iOS versions prior to 13.
#if !defined(TARGET_OS_IOS) || __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
        // libcurl will not automatically create the target directory when
        // PUTting to a `file:///` URL. So we do that manually here.
        if (pThis->_options.allowDirectoryCreation && isFile(pRequest->url())) {
          std::filesystem::path filePath =
              convertFileUriToFilename(pRequest->url());
          if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
          }
        }
#endif

        CurlHandle curl(pThis.get());

        curl_slist* list = setCommonOptions(
            curl(),
            pRequest->url(),
            pRequest->headers(),
            pThis->_options.userAgent,
            pThis->_options.certificatePath,
            pThis->_options.certificateFile);

        curl_easy_setopt(curl(), verbOption, 1);
        curl_easy_setopt(
            curl(),
            CURLOPT_READFUNCTION,
            CurlAssetRequest::uploadDataCallback);
        curl_easy_setopt(curl(), CURLOPT_READDATA, pRequest.get());
        curl_easy_setopt(
            curl(),
            CURLOPT_INFILESIZE_LARGE,
            curl_off_t(pRequest->contentPayload().size()));
        curl_easy_setopt(
            curl(),
            CURLOPT_FTP_CREATE_MISSING_DIRS,
            CURLFTP_CREATE_DIR);

        std::unique_ptr<CurlAssetResponse> pResponse =
            std::make_unique<CurlAssetResponse>();
        pResponse->setCallbacks(curl());
        CURLcode responseCode = curl_easy_perform(curl());
        curl_slist_free_all(list);
        if (responseCode == 0) {
          // Use `long` instead of int64_t to match the documented
          // `CURLINFO_RESPONSE_CODE` type.
          // NOLINTNEXTLINE(google-runtime-int)
          long httpResponseCode = 0;
          curl_easy_getinfo(curl(), CURLINFO_RESPONSE_CODE, &httpResponseCode);
          pResponse->_statusCode = static_cast<uint16_t>(httpResponseCode);
          char* ct = nullptr;
          curl_easy_getinfo(curl(), CURLINFO_CONTENT_TYPE, &ct);
          if (ct) {
            pResponse->_contentType = ct;
          }
          pRequest->setResponse(std::move(pResponse));
          return pRequest;
        } else {
          throw std::runtime_error(fmt::format(
              "{} `{}` failed: {}",
              pRequest->method(),
              pRequest->url(),
              curl_easy_strerror(responseCode)));
        }
      });
}

void CurlAssetAccessor::tick() noexcept {}

} // namespace CesiumCurl
