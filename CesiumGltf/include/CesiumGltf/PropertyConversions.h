#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace CesiumGltf {
template <
    typename TSigned,
    std::enable_if_t<
        IsMetadataInteger<TSigned>::value && std::is_signed_v<TSigned>>>
double normalize(TSigned value) {
  double max = static_cast<double>(std::numeric_limits<TSigned>::max());
  return std::max(static_cast<double>(value) / max, -1.0);
}

template <
    typename TUnsigned,
    std::enable_if_t<
        IsMetadataInteger<TUnsigned>::value && std::is_unsigned_v<TUnsigned>>>
double normalize(TUnsigned value) {
  double max = static_cast<double>(std::numeric_limits<TUnsigned>::max());
  return static_cast<double>(value) / max;
}

template <
    glm::length_t N,
    typename TSigned,
    glm::qualifier Q,
    std::enable_if_t<
        IsMetadataInteger<TSigned>::value && std::is_signed_v<TSigned>>>
glm::vec<N, double, Q> normalize(glm::vec<N, TSigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TSigned>::max());
  return glm::max(static_cast<glm::vec<N, double, Q>>(value) / max, -1.0);
}

template <
    glm::length_t N,
    typename TUnsigned,
    glm::qualifier Q,
    std::enable_if_t<
        IsMetadataInteger<TUnsigned>::value && std::is_unsigned_v<TUnsigned>>>
glm::mat<N, N, double, Q> normalize(glm::mat<N, N, TUnsigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TUnsigned>::max());
  return glm::max(static_cast<glm::mat<N, N, double, Q>>(value) / max, -1.0);
}

} // namespace CesiumGltf
