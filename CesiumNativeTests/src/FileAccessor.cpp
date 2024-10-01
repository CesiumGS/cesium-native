#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumUtility/Uri.h>

#include <cstring>
#include <fstream>

namespace CesiumNativeTests {

namespace {
std::unique_ptr<SimpleAssetResponse> readFileUri(const std::string& uri) {
  std::vector<std::byte> result;
  CesiumAsync::HttpHeaders headers;
  std::string contentType;
  auto response = [&](uint16_t errorCode) {
    return std::make_unique<SimpleAssetResponse>(
        errorCode,
        contentType,
        headers,
        std::move(result));
  };
  auto protocolPos = uri.find("file:///");
  if (protocolPos != 0) {
    return response(400);
  }
  std::string path =
      CesiumUtility::Uri::uriPathToNativePath(CesiumUtility::Uri::getPath(uri));
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return response(404);
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  result.resize(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(result.data()), size);
  if (!file) {
    return response(503);
  } else {
    contentType = "application/octet-stream";
    headers.insert({"content-type", contentType});
    return response(200);
  }
}
} // namespace

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
FileAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [&](const auto& promise) {
        auto response = readFileUri(url);
        CesiumAsync::HttpHeaders cesiumHeaders(headers.begin(), headers.end());
        auto request = std::make_shared<SimpleAssetRequest>(
            "GET",
            url,
            cesiumHeaders,
            std::move(response));
        promise.resolve(request);
      });
}

// Can we do anything with a request that isn't a GET?
CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
FileAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const gsl::span<const std::byte>&) {
  if (verb == "GET") {
    return get(asyncSystem, url, headers);
  }
  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [&](const auto& promise) {
        promise.reject(std::runtime_error("unsupported operation"));
      });
}
} // namespace CesiumNativeTests
