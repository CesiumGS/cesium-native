#pragma once

#include <Cesium3DTilesSelection/Exp_GltfConverterResult.h>
#include <GSL/span>
#include <string>
#include <string_view>
#include <optional>

namespace Cesium3DTilesSelection {
class GltfConverters {
public:
  using ConverterFun =
      GltfConverterResult (*)(const gsl::span<const std::byte>& content);

  static void
  registerMagic(const std::string& magic, ConverterFun converter);

  static void registerFileExtension(
      const std::string& fileExtension,
      ConverterFun converter);

  static GltfConverterResult convert(
      const gsl::span<const std::byte>& content,
      const std::string& filePath);

  static GltfConverterResult convert(const gsl::span<const std::byte>& content);

private:
  static std::string toLowerCase(const std::string_view& str);

  static std::unordered_map<std::string, ConverterFun> _loadersByMagic;
  static std::unordered_map<std::string, ConverterFun> _loadersByFileExtension;
};
} // namespace Cesium3DTilesSelection
