#pragma once

#include <Cesium3DTilesSelection/Exp_GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>
#include <string>
#include <string_view>

namespace Cesium3DTilesSelection {
class GltfConverters {
public:
  using ConverterFun = GltfConverterResult (*)(
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

  static void registerMagic(const std::string& magic, ConverterFun converter);

  static void registerFileExtension(
      const std::string& fileExtension,
      ConverterFun converter);

  static ConverterFun getConverterByFileExtension(const std::string& filePath);

  static ConverterFun
  getConverterByMagic(const gsl::span<const std::byte>& content);

  static GltfConverterResult convert(
      const std::string& filePath,
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

  static GltfConverterResult convert(
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

private:
  static std::string toLowerCase(const std::string_view& str);

  static std::string getFileExtension(const std::string_view& filePath);

  static std::unordered_map<std::string, ConverterFun> _loadersByMagic;
  static std::unordered_map<std::string, ConverterFun> _loadersByFileExtension;
};
} // namespace Cesium3DTilesSelection
