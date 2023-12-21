#pragma once

#include "AccessorView.h"

#include <glm/common.hpp>

#include <variant>

namespace CesiumGltf {
/**
 * @brief Visitor that retrieves the count of elements in the given accessor
 * type as an int64_t.
 */
struct CountFromAccessor {
  int64_t operator()(std::monostate) { return 0; }

  template <typename T> int64_t operator()(const AccessorView<T>& value) {
    return value.size();
  }
};

/**
 * @brief Visitor that retrieves the status from the given accessor. Returns an
 * invalid status for a std::monostate (interpreted as a nonexistent accessor).
 */
struct StatusFromAccessor {
  AccessorViewStatus operator()(std::monostate) {
    return AccessorViewStatus::InvalidAccessorIndex;
  }

  template <typename T>
  AccessorViewStatus operator()(const AccessorView<T>& value) {
    return value.status();
  }
};

/**
 * Type definition for all kinds of feature ID attribute accessors.
 */
typedef std::variant<
    AccessorView<int8_t>,
    AccessorView<uint8_t>,
    AccessorView<int16_t>,
    AccessorView<uint16_t>,
    AccessorView<uint32_t>,
    AccessorView<float>>
    FeatureIdAccessorType;

/**
 * Retrieves an accessor view for the specified feature ID attribute from the
 * given glTF primitive and model. This verifies that the accessor is of a valid
 * type. If not, the returned accessor view will be invalid.
 */
FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t featureIdAttributeIndex);

/**
 * Visitor that retrieves the feature ID from the given accessor type as an
 * int64_t. This should be initialized with the index of the vertex whose
 * feature ID is being queried.
 *
 * -1 is used to indicate errors retrieving the feature ID, e.g., if the given
 * index was out-of-bounds.
 */
struct FeatureIdFromAccessor {
  int64_t operator()(const AccessorView<float>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64_t>(glm::round(value[index]));
  }

  template <typename T> int64_t operator()(const AccessorView<T>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64_t>(value[index]);
  }

  int64_t index;
};

/**
 * Type definition for all kinds of index accessors. std::monostate
 * indicates a nonexistent accessor, which can happen (and is valid) if the
 * primitive vertices are defined without an index buffer.
 */
typedef std::variant<
    std::monostate,
    AccessorView<uint8_t>,
    AccessorView<uint16_t>,
    AccessorView<uint32_t>>
    IndexAccessorType;

/**
 * Retrieves an accessor view for the indices of the given glTF primitive from
 * the model. The primitive may not specify any indices; if so, std::monostate
 * is returned.
 */
IndexAccessorType
getIndexAccessorView(const Model& model, const MeshPrimitive& primitive);

/**
 * Visitor that retrieves the vertex indices from the given accessor type
 * corresponding to a given face index. These indices are returned as an array
 * of int64_ts. This should be initialized with the index of the face and the
 * total number of vertices in the primitive.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct IndicesForFaceFromAccessor {
  std::array<int64_t, 3> operator()(std::monostate) {
    if (faceIndex < 0 || faceIndex >= vertexCount / 3) {
      return {-1, -1, -1};
    }

    const int64_t firstVertex = faceIndex * 3;
    std::array<int64_t, 3> result;

    for (int64_t i = 0; i < 3; i++) {
      int64_t vertexIndex = firstVertex + i;
      result[i] = vertexIndex < vertexCount ? vertexIndex : -1;
    }

    return result;
  }

  template <typename T>
  std::array<int64_t, 3> operator()(const AccessorView<T>& value) {
    if (faceIndex < 0 || faceIndex >= value.size() / 3) {
      return {-1, -1, -1};
    }

    const int64_t firstVertex = faceIndex * 3;
    std::array<int64_t, 3> result;

    for (int64_t i = 0; i < 3; i++) {
      int64_t vertexIndex = firstVertex + i;
      result[i] = vertexIndex < value.size()
                      ? static_cast<int64_t>(value[vertexIndex])
                      : -1;
    }

    return result;
  }

  int64_t faceIndex;
  int64_t vertexCount;
};

/**
 * Type definition for all kinds of texture coordinate (TEXCOORD_n) accessors.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC2<uint8_t>>,
    AccessorView<AccessorTypes::VEC2<uint16_t>>,
    AccessorView<AccessorTypes::VEC2<float>>>
    TexCoordAccessorType;

/**
 * Retrieves an accessor view for the specified texture coordinate set from the
 * given glTF primitive and model. This verifies that the accessor is of a valid
 * type. If not, the returned accessor view will be invalid.,
 */
TexCoordAccessorType getTexCoordAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t textureCoordinateSetIndex);

/**
 * Visitor that retrieves the texture coordinates from the given accessor type
 * as a glm::dvec2. This should be initialized with the target index.
 *
 * There are technically no invalid UV values because of clamp / wrap
 * behavior, so we use std::nullopt to denote an erroneous value.
 */
struct TexCoordFromAccessor {
  std::optional<glm::dvec2>
  operator()(const AccessorView<AccessorTypes::VEC2<float>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    return glm::dvec2(value[index].value[0], value[index].value[1]);
  }

  template <typename T>
  std::optional<glm::dvec2>
  operator()(const AccessorView<AccessorTypes::VEC2<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    double u = static_cast<double>(value[index].value[0]);
    double v = static_cast<double>(value[index].value[1]);

    // TODO: do normalization logic in accessor view?
    u /= std::numeric_limits<T>::max();
    v /= std::numeric_limits<T>::max();

    return glm::dvec2(u, v);
  }

  int64_t index;
};

} // namespace CesiumGltf
