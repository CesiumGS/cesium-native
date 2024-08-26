#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace CesiumGltf {
template <typename T> double normalize(T value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  if constexpr (std::is_signed_v<T>) {
    return std::max(static_cast<double>(value) / max, -1.0);
  } else {
    return static_cast<double>(value) / max;
  }
}

template <glm::length_t N, typename T>
glm::vec<N, double> normalize(glm::vec<N, T> value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  if constexpr (std::is_signed_v<T>) {
    return glm::max(static_cast<glm::vec<N, double>>(value) / max, -1.0);
  } else {
    return static_cast<glm::vec<N, double>>(value) / max;
  }
}

template <glm::length_t N, typename T>
glm::mat<N, N, double> normalize(glm::mat<N, N, T> value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  // No max() implementation for matrices, so we have to write our own.
  glm::mat<N, N, double> result;
  for (glm::length_t i = 0; i < N; i++) {
    for (glm::length_t j = 0; j < N; j++) {
      if constexpr (std::is_signed_v<T>) {
        result[i][j] = glm::max(static_cast<double>(value[i][j]) / max, -1.0);
      } else {
        result[i][j] = static_cast<double>(value[i][j]) / max;
      }
    }
  }
  return result;
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
T transformValue(
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
PropertyArrayCopy<T> transformArray(
    const PropertyArrayView<T>& value,
    const std::optional<PropertyArrayView<T>>& offset,
    const std::optional<PropertyArrayView<T>>& scale) {
  std::vector<T> result(static_cast<size_t>(value.size()));
  for (int64_t i = 0; i < value.size(); i++) {
    result[i] = value[i];

    if (scale) {
      result[i] = applyScale<T>(result[i], (*scale)[i]);
    }

    if (offset) {
      result[i] = result[i] + (*offset)[i];
    }
  }

  return PropertyArrayCopy(std::move(result));
}

template <
    typename T,
    typename NormalizedType = typename TypeToNormalizedType<T>::type>
PropertyArrayCopy<NormalizedType> transformNormalizedArray(
    const PropertyArrayView<T>& value,
    const std::optional<PropertyArrayView<NormalizedType>>& offset,
    const std::optional<PropertyArrayView<NormalizedType>>& scale) {
  std::vector<NormalizedType> result(static_cast<size_t>(value.size()));
  for (int64_t i = 0; i < value.size(); i++) {
    result[i] = normalize<T>(value[i]);

    if (scale) {
      result[i] = result[i] * (*scale)[i];
    }

    if (offset) {
      result[i] = result[i] + (*offset)[i];
    }
  }

  return PropertyArrayCopy(std::move(result));
}

template <glm::length_t N, typename T>
PropertyArrayCopy<glm::vec<N, double>> transformNormalizedVecNArray(
    const PropertyArrayView<glm::vec<N, T>>& value,
    const std::optional<PropertyArrayView<glm::vec<N, double>>>& offset,
    const std::optional<PropertyArrayView<glm::vec<N, double>>>& scale) {
  std::vector<glm::vec<N, double>> result(static_cast<size_t>(value.size()));
  for (int64_t i = 0; i < value.size(); i++) {
    result[i] = normalize<N, T>(value[i]);

    if (scale) {
      result[i] = result[i] * (*scale)[i];
    }

    if (offset) {
      result[i] = result[i] + (*offset)[i];
    }
  }

  return PropertyArrayCopy(std::move(result));
}

template <glm::length_t N, typename T>
PropertyArrayCopy<glm::mat<N, N, double>> transformNormalizedMatNArray(
    const PropertyArrayView<glm::mat<N, N, T>>& value,
    const std::optional<PropertyArrayView<glm::mat<N, N, double>>>& offset,
    const std::optional<PropertyArrayView<glm::mat<N, N, double>>>& scale) {
  std::vector<glm::mat<N, N, double>> result(static_cast<size_t>(value.size()));
  for (int64_t i = 0; i < value.size(); i++) {
    result[i] = normalize<N, T>(value[i]);

    if (scale) {
      result[i] = applyScale<glm::mat<N, N, double>>(result[i], (*scale)[i]);
    }

    if (offset) {
      result[i] = result[i] + (*offset)[i];
    }
  }

  return PropertyArrayCopy(std::move(result));
}
} // namespace CesiumGltf
