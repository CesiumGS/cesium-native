#pragma once

#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTypeTraits.h>

#include <glm/common.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace CesiumGltf {
/**
 * @brief Normalizes the given value between [0, 1] if unsigned or [-1, 1] if
 * signed, based on the type's maximum value.
 *
 * @param value The value to normalize.
 */
template <typename T> double normalize(T value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  if constexpr (std::is_signed_v<T>) {
    return std::max(static_cast<double>(value) / max, -1.0);
  } else {
    return static_cast<double>(value) / max;
  }
}

/**
 * @brief Normalizes the given vector's components between [0, 1] if unsigned or
 * [-1, 1] if signed, based on the type's maximum value.
 *
 * @param value The value to normalize.
 */
template <glm::length_t N, typename T>
glm::vec<N, double> normalize(glm::vec<N, T> value) {
  constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
  if constexpr (std::is_signed_v<T>) {
    return glm::max(static_cast<glm::vec<N, double>>(value) / max, -1.0);
  } else {
    return static_cast<glm::vec<N, double>>(value) / max;
  }
}

/**
 * @brief Normalizes the given matrix's components between [0, 1] if unsigned or
 * [-1, 1] if signed, based on the type's maximum value.
 *
 * @param value The value to normalize.
 */
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

/**
 * @brief Multiplies each component of the value by the given scale factor.
 *
 * @param value The value to scale.
 * @param scale The scale factor to apply to the value.
 */
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

/**
 * @brief Transforms the value by optional offset and scale factors.
 *
 * @param value The value to transform.
 * @param offset The amount to offset the value by, or `std::nullopt` to apply
 * no offset.
 * @param scale The amount to scale the value by, or `std::nullopt` to apply no
 * scale. See \ref applyScale.
 */
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

/**
 * @brief Transforms each element of an array of values by optional offset and
 * scale factors. See \ref transformValue.
 *
 * @param value The array whose elements will be transformed.
 * @param offset The amount to offset each element by, or `std::nullopt` to
 * apply no offset.
 * @param scale The amount to scale each element by, or `std::nullopt` to apply
 * no scale factor.
 * @returns A transformed copy of the input array.
 */
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

/**
 * @brief Normalizes each element of an array of values and transforms them by
 * optional offset and scale factors. See \ref transformValue and \ref
 * transformArray.
 *
 * @param value The array whose elements will be transformed.
 * @param offset The amount to offset each element by, or `std::nullopt` to
 * apply no offset. The offset will be applied after normalization.
 * @param scale The amount to scale each element by, or `std::nullopt` to apply
 * no scale factor. The scale will be applied after normalization.
 * @returns A normalized and transformed copy of the input array.
 */
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

/**
 * @brief Normalizes each element of an array of vectors and transforms them by
 * optional offset and scale factors. See \ref transformNormalizedArray.
 *
 * @param value The array whose elements will be transformed.
 * @param offset The amount to offset each element by, or `std::nullopt` to
 * apply no offset. The offset will be applied after normalization.
 * @param scale The amount to scale each element by, or `std::nullopt` to apply
 * no scale factor. The scale will be applied after normalization.
 * @returns A normalized and transformed copy of the input array.
 */
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

/**
 * @brief Normalizes each element of an array of matrices and transforms them by
 * optional offset and scale factors. See \ref transformNormalizedArray.
 *
 * @param value The array whose elements will be transformed.
 * @param offset The amount to offset each element by, or `std::nullopt` to
 * apply no offset. The offset will be applied after normalization.
 * @param scale The amount to scale each element by, or `std::nullopt` to apply
 * no scale factor. The scale will be applied after normalization.
 * @returns A normalized and transformed copy of the input array.
 */
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
