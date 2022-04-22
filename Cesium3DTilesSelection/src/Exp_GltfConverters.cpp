#include <Cesium3DTilesSelection/Exp_GltfConverters.h>

#include <spdlog/spdlog.h>

namespace Cesium3DTilesSelection {
std::unordered_map<std::string, GltfConverters::ConverterFun>
    GltfConverters::_loadersByMagic;

std::unordered_map<std::string, GltfConverters::ConverterFun>
    GltfConverters::_loadersByFileExtension;

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

GltfConverters::ConverterFun
GltfConverters::getConverterByFileExtension(const std::string& filePath) {
  std::string extension = getFileExtension(filePath);
  auto itExtension = _loadersByFileExtension.find(extension);
  if (itExtension != _loadersByFileExtension.end()) {
    return itExtension->second;
  }

  return nullptr;
}

GltfConverters::ConverterFun
GltfConverters::getConverterByMagic(const gsl::span<const std::byte>& content) {
  if (content.size() >= 4) {
    std::string magic(reinterpret_cast<const char*>(content.data()), 4);
    auto converterIter = _loadersByMagic.find(magic);
    if (converterIter != _loadersByMagic.end()) {
      return converterIter->second;
    }
  }

  return nullptr;
}

GltfConverterResult GltfConverters::convert(
    const std::string& filePath,
    const gsl::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options) {
  auto converterFun = getConverterByMagic(content);
  if (converterFun) {
    return converterFun(content, options);
  }

  converterFun = getConverterByFileExtension(filePath);
  if (converterFun) {
    return converterFun(content, options);
  }

  return GltfConverterResult{};
}

GltfConverterResult GltfConverters::convert(
    const gsl::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options) {
  auto converter = getConverterByMagic(content);
  if (converter) {
    return converter(content, options);
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

std::string GltfConverters::getFileExtension(const std::string_view& filePath) {
  std::string_view urlWithoutQueries = filePath.substr(0, filePath.find("?"));
  size_t extensionPos = urlWithoutQueries.rfind(".");
  if (extensionPos < urlWithoutQueries.size()) {
    std::string_view extension = urlWithoutQueries.substr(extensionPos);
    std::string lowerCaseExtension = toLowerCase(extension);
    return lowerCaseExtension;
  }

  return "";
}
} // namespace Cesium3DTilesSelection
