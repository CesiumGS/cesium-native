#include <Cesium3DTilesSelection/Exp_GltfConverters.h>

#include <spdlog/spdlog.h>

namespace Cesium3DTilesSelection {
void GltfConverters::registerMagic(
    const std::string& magic,
    ConverterFun converter) {
  SPDLOG_INFO("Registering magic header {}", magic);
  _loadersByMagic[magic] = converter;
}

void GltfConverters::registerFileExtension(
    const std::string& fileExtension,
    ConverterFun converter) {
  SPDLOG_INFO("Registering file extension {}", fileExtension);

  std::string lowerCaseFileExtension = toLowerCase(fileExtension);
  _loadersByFileExtension[lowerCaseFileExtension] = converter;
}

GltfConverterResult GltfConverters::convert(
    const gsl::span<const std::byte>& content,
    const std::string& filePath) {
  // using magic first
  if (content.size() >= 4) {
    std::string magic(reinterpret_cast<const char*>(content.data()), 4);
    auto converterIter = _loadersByMagic.find(magic);
    if (converterIter != _loadersByMagic.end()) {
      return converterIter->second(content);
    }
  }

  // if magic content is not possible, use file extensions
  std::string_view url = filePath;
  std::string_view urlWithoutQueries = url.substr(0, url.find("?"));
  size_t extensionPos = urlWithoutQueries.rfind(".");
  if (extensionPos < urlWithoutQueries.size()) {
    std::string_view extension = urlWithoutQueries.substr(extensionPos);
    std::string lowerCaseExtension = toLowerCase(extension);
    auto itExtension = _loadersByFileExtension.find(lowerCaseExtension);
    if (itExtension != _loadersByFileExtension.end()) {
      return itExtension->second(content);
    }
  }

  return GltfConverterResult{};
}

GltfConverterResult
GltfConverters::convert(const gsl::span<const std::byte>& content) {
  if (content.size() >= 4) {
    std::string magic(reinterpret_cast<const char*>(content.data()), 4);
    auto converterIter = _loadersByMagic.find(magic);
    if (converterIter != _loadersByMagic.end()) {
      return converterIter->second(content);
    }
  }

  return GltfConverterResult{};
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
} // namespace Cesium3DTilesSelection
