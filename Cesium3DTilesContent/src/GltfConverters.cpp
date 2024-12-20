#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

using namespace CesiumUtility;

namespace Cesium3DTilesContent {
std::unordered_map<std::string, GltfConverters::ConverterFunction>
    GltfConverters::_loadersByMagic;

std::unordered_map<std::string, GltfConverters::ConverterFunction>
    GltfConverters::_loadersByFileExtension;

void GltfConverters::registerMagic(
    const std::string& magic,
    ConverterFunction converter) {
  _loadersByMagic[magic] = converter;
}

void GltfConverters::registerFileExtension(
    const std::string& fileExtension,
    ConverterFunction converter) {

  std::string lowerCaseFileExtension = toLowerCase(fileExtension);
  _loadersByFileExtension[lowerCaseFileExtension] = converter;
}

GltfConverters::ConverterFunction
GltfConverters::getConverterByFileExtension(const std::string& filePath) {
  std::string extension;
  return getConverterByFileExtension(filePath, extension);
}

GltfConverters::ConverterFunction
GltfConverters::getConverterByMagic(const std::span<const std::byte>& content) {
  std::string magic;
  return getConverterByMagic(content, magic);
}

CesiumAsync::Future<GltfConverterResult> GltfConverters::convert(
    const std::string& filePath,
    const std::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  std::string magic;
  auto converterFun = getConverterByMagic(content, magic);
  if (converterFun) {
    return converterFun(content, options, assetFetcher);
  }

  std::string fileExtension;
  converterFun = getConverterByFileExtension(filePath, fileExtension);
  if (converterFun) {
    return converterFun(content, options, assetFetcher);
  }

  ErrorList errors;
  errors.emplaceError(fmt::format(
      "No loader registered for tile with content type '{}' and magic value "
      "'{}'",
      fileExtension,
      magic));

  return assetFetcher.asyncSystem.createResolvedFuture(
      GltfConverterResult{std::nullopt, std::move(errors)});
}

CesiumAsync::Future<GltfConverterResult> GltfConverters::convert(
    const std::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  std::string magic;
  auto converter = getConverterByMagic(content, magic);
  if (converter) {
    return converter(content, options, assetFetcher);
  }

  ErrorList errors;
  errors.emplaceError(fmt::format(
      "No loader registered for tile with magic value '{}'",
      magic));

  return assetFetcher.asyncSystem.createResolvedFuture(
      GltfConverterResult{std::nullopt, std::move(errors)});
}

std::string GltfConverters::toLowerCase(const std::string_view& str) {
  std::string result;
  std::transform(
      str.begin(),
      str.end(),
      std::back_inserter(result),
      [](char c) noexcept { return static_cast<char>(::tolower(c)); });

  return result;
}

std::string GltfConverters::getFileExtension(const std::string_view& filePath) {
  std::string_view urlWithoutQueries = filePath.substr(0, filePath.find('?'));
  size_t extensionPos = urlWithoutQueries.rfind('.');
  if (extensionPos < urlWithoutQueries.size()) {
    std::string_view extension = urlWithoutQueries.substr(extensionPos);
    std::string lowerCaseExtension = toLowerCase(extension);
    return lowerCaseExtension;
  }

  return "";
}

GltfConverters::ConverterFunction GltfConverters::getConverterByFileExtension(
    const std::string& filePath,
    std::string& extension) {
  extension = getFileExtension(filePath);
  auto itExtension = _loadersByFileExtension.find(extension);
  if (itExtension != _loadersByFileExtension.end()) {
    return itExtension->second;
  }

  return nullptr;
}

GltfConverters::ConverterFunction GltfConverters::getConverterByMagic(
    const std::span<const std::byte>& content,
    std::string& magic) {
  if (content.size() >= 4) {
    magic = std::string(reinterpret_cast<const char*>(content.data()), 4);
    auto converterIter = _loadersByMagic.find(magic);
    if (converterIter != _loadersByMagic.end()) {
      return converterIter->second;
    }
  }

  return nullptr;
}

CesiumAsync::Future<AssetFetcherResult>
AssetFetcher::get(const std::string& relativeUrl) const {
  auto resolvedUrl = Uri::resolve(baseUrl, relativeUrl);
  return pAssetAccessor->get(asyncSystem, resolvedUrl, requestHeaders)
      .thenImmediately(
          [asyncSystem = asyncSystem](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            AssetFetcherResult assetFetcherResult;
            const auto& url = pCompletedRequest->url();
            if (!pResponse) {
              assetFetcherResult.errorList.emplaceError(fmt::format(
                  "Did not receive a valid response for asset {}",
                  url));
              return asyncSystem.createResolvedFuture(
                  std::move(assetFetcherResult));
            }
            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              assetFetcherResult.errorList.emplaceError(fmt::format(
                  "Received status code {} for asset {}",
                  statusCode,
                  url));
              return asyncSystem.createResolvedFuture(
                  std::move(assetFetcherResult));
            }
            std::span<const std::byte> asset = pResponse->data();
            std::copy(
                asset.begin(),
                asset.end(),
                std::back_inserter(assetFetcherResult.bytes));
            return asyncSystem.createResolvedFuture(
                std::move(assetFetcherResult));
          });
}
} // namespace Cesium3DTilesContent
