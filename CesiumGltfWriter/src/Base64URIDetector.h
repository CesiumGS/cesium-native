#pragma once

#include <string_view>

namespace CesiumGltfWriter {
constexpr auto BASE64_PREFIX = "data:application/octet-stream;base64,";
[[nodiscard]] inline bool isURIBase64DataURI(std::string_view view) noexcept {
  return view.rfind(BASE64_PREFIX, 0) == 0;
}
} // namespace CesiumGltfWriter
