#pragma once
#include <CesiumI3S/Library.h>

#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Resource reference used in NodeIndexDocument legacy payloads
 * (resource.cmn.md). */
struct CESIUMI3S_API Resource {
  /** @brief Relative URL to the resource. */
  std::string href;
  /** @brief Layer content type strings (deprecated, I3S 1.6). */
  std::optional<std::vector<std::string>> layerContent;
  /** @brief Feature ID range covered by this resource (deprecated, I3S 1.6). */
  std::optional<std::vector<double>> featureRange;
  /** @brief Multi-texture bundle identifier (deprecated, I3S 1.6). */
  std::optional<std::string> multiTextureBundle;
  /** @brief Vertex element ranges (deprecated, I3S 1.6). */
  std::optional<std::vector<double>> vertexElements;
  /** @brief Face element ranges (deprecated, I3S 1.6). */
  std::optional<std::vector<double>> faceElements;
};

} // namespace CesiumI3S
