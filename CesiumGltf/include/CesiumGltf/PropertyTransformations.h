#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace CesiumGltf {
template <typename T>
double normalize(T value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  if constexpr (std::is_signed_v<T>) {
    return std::max(static_cast<double>(value) / max, -1.0);
  } else {
    return static_cast<double>(value) / max;
  }
}

template <
    glm::length_t N,
    typename TSigned,
    glm::qualifier Q,
    std::enable_if_t<std::is_signed_v<TSigned>>>
glm::vec<N, double, Q> normalize(glm::vec<N, TSigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TSigned>::max());
  return glm::max(static_cast<glm::vec<N, double, Q>>(value) / max, -1.0);
}

template <
    glm::length_t N,
    typename TUnsigned,
    glm::qualifier Q,
    std::enable_if_t<std::is_unsigned_v<TUnsigned>>>
glm::vec<N, double, Q> normalize(glm::vec<N, TUnsigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TUnsigned>::max());
  return static_cast<glm::vec<N, double, Q>>(value) / max;
}

template <
    glm::length_t N,
    typename TSigned,
    glm::qualifier Q,
    std::enable_if_t<std::is_signed_v<TSigned>>>
glm::mat<N, N, double, Q> normalize(glm::mat<N, N, TSigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TSigned>::max());
  return glm::max(static_cast<glm::mat<N, N, double, Q>>(value) / max, -1.0);
}

template <
    glm::length_t N,
    typename TUnsigned,
    glm::qualifier Q,
    std::enable_if_t<std::is_unsigned_v<TUnsigned>>>
glm::mat<N, N, double, Q> normalize(glm::mat<N, N, TUnsigned, Q> value) {
  double max = static_cast<double>(std::numeric_limits<TUnsigned>::max());
  return static_cast<glm::mat<N, N, double, Q>>(value) / max;
}

template <typename T> T applyScale(const T& value, const T& scale) {
  if constexpr (IsMetadataMatN<T>::value) {
    // Do component-wise multiplication instead of actual matrix
    // multiplication.
    T matN;
    constexpr glm::length_t N = T::length();
    for (glm::length_t i = 0; i < N; i++) {
      matN[i] = value[i] * scale[i];
    }
    return matN;
  } else {
    return value * scale;
  }
}

template <typename T>
T applyOffsetAndScale(
    const T& value,
    const std::optional<T>& offset,
    const std::optional<T>& scale) {
  T result = value;
  if (scale) {
    result = applyScale<T>(result, *scale);
  }

  if (offset) {
    result += *offset;
  }

  return result;
}

template <typename T>
PropertyArrayView<T> applyOffsetAndScale(
    const PropertyArrayView<T>& value,
    const std::optional<PropertyArrayView<T>>& offset,
    const std::optional<PropertyArrayView<T>>& scale) {
  int64_t offsetSize = offset ? offset->size() : 0;
  int64_t scaleSize = scale ? scale->size() : 0;
  std::vector<T> result(static_cast<size_t>(value.size()));
  for (int64_t i = 0; i < value.size(); i++) {
    result[i] = value[i];

    if (i < scaleSize) {
      result = applyScale(result[i], scale[i]);
    }

    if (i < offsetSize) {
      result[i] = result[i] + offset[i];
    }
  }

  return PropertyArrayView<T>(std::move(result));
}

} // namespace CesiumGltf
