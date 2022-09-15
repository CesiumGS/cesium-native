#include <Cesium3DTilesSelection/GltfConverters.h>

#include <spdlog/spdlog.h>

namespace Cesium3DTilesSelection {
std::unordered_map<std::string, GltfConverters::ConverterFunction>
    GltfConverters::_loadersByMagic;

std::unordered_map<std::string, GltfConverters::ConverterFunction>
    GltfConverters::_loadersByFileExtension;

void GltfConverters::registerMagic(
    const std::string& magic,
    ConverterFunction converter) {
  SPDLOG_INFO("Registering magic header {}", magic);
  _loadersByMagic[magic] = converter;
}

void GltfConverters::registerFileExtension(
    const std::string& fileExtension,
    ConverterFunction converter) {
  SPDLOG_INFO("Registering file extension {}", fileExtension);

  std::string lowerCaseFileExtension = toLowerCase(fileExtension);
  _loadersByFileExtension[lowerCaseFileExtension] = converter;
}

GltfConverters::ConverterFunction
GltfConverters::getConverterByFileExtension(const std::string& filePath) {
  std::string extension;
  return getConverterByFileExtension(filePath, extension);
}

GltfConverters::ConverterFunction
GltfConverters::getConverterByMagic(const gsl::span<const std::byte>& content) {
  std::string magic;
  return getConverterByMagic(content, magic);
}

GltfConverterResult GltfConverters::convert(
    const std::string& filePath,
    const gsl::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options) {
  std::string magic;
  auto converterFun = getConverterByMagic(content, magic);
  if (converterFun) {
    return converterFun(content, options);
  }

  std::string fileExtension;
  converterFun = getConverterByFileExtension(filePath, fileExtension);
  if (converterFun) {
    return converterFun(content, options);
  }

  ErrorList errors;
  errors.emplaceError(fmt::format(
      "No loader registered for tile with content type '{}' and magic value "
      "'{}'",
      fileExtension,
      magic));

  return GltfConverterResult{std::nullopt, std::move(errors)};
}

GltfConverterResult GltfConverters::convert(
    const gsl::span<const std::byte>& content,
    const CesiumGltfReader::GltfReaderOptions& options) {
  std::string magic;
  auto converter = getConverterByMagic(content, magic);
  if (converter) {
    return converter(content, options);
  }

  ErrorList errors;
  errors.emplaceError(fmt::format(
      "No loader registered for tile with magic value '{}'",
      magic));

  return GltfConverterResult{std::nullopt, std::move(errors)};
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
    const gsl::span<const std::byte>& content,
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
} // namespace Cesium3DTilesSelection
