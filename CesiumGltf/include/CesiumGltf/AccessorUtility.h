#pragma once

#include "AccessorView.h"

#include <CesiumGltf/MeshPrimitive.h>

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
 * Type definition for position accessor.
 */
typedef AccessorView<AccessorTypes::VEC3<float>> PositionAccessorType;

/**
 * Retrieves an accessor view for the position attribute from the given glTF
 * primitive and model. This verifies that the accessor is of a valid type. If
 * not, the returned accessor view will be invalid.
 */
PositionAccessorType
getPositionAccessorView(const Model& model, const MeshPrimitive& primitive);

/**
 * Type definition for normal accessor.
 */
typedef AccessorView<AccessorTypes::VEC3<float>> NormalAccessorType;

/**
 * Retrieves an accessor view for the normal attribute from the given glTF
 * primitive and model. This verifies that the accessor is of a valid type. If
 * not, the returned accessor view will be invalid.
 */
NormalAccessorType
getNormalAccessorView(const Model& model, const MeshPrimitive& primitive);

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
 * Retrieves an accessor view for the specified feature ID attribute from the
 * given glTF node and model, if the node contains an EXT_mesh_gpu_instancing
 * property. This verifies that the accessor is of a valid type. If not, the
 * returned accessor view will be invalid.
 */
FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const Node& node,
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
 * of int64_ts. This should be initialized with the index of the face, the
 * total number of vertices in the primitive, and the
 * `CesiumGltf::MeshPrimitive::Mode` of the primitive.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct IndicesForFaceFromAccessor {
  std::array<int64_t, 3> operator()(std::monostate) {
    int64_t firstVertex = faceIndex;
    int64_t numFaces = 0;

    switch (primitiveMode) {
    case MeshPrimitive::Mode::TRIANGLE_STRIP:
      numFaces = vertexCount - 2;
      break;
    case MeshPrimitive::Mode::TRIANGLE_FAN:
      numFaces = vertexCount - 2;
      firstVertex++;
      break;
    case MeshPrimitive::Mode::TRIANGLES:
      numFaces = vertexCount / 3;
      firstVertex *= 3;
      break;
    default:
      // Unsupported primitive mode.
      return {-1, -1, -1};
    }

    if (faceIndex < 0 || faceIndex >= numFaces) {
      return {-1, -1, -1};
    }

    std::array<int64_t, 3> result;

    if (primitiveMode == MeshPrimitive::Mode::TRIANGLE_FAN) {
      result[0] = 0;
      result[1] = firstVertex < vertexCount ? firstVertex : -1;
      result[2] = firstVertex + 1 < vertexCount ? firstVertex + 1 : -1;
    } else {
      for (int64_t i = 0; i < 3; i++) {
        int64_t vertexIndex = firstVertex + i;
        result[i] = vertexIndex < vertexCount ? vertexIndex : -1;
      }
    }

    return result;
  }

  template <typename T>
  std::array<int64_t, 3> operator()(const AccessorView<T>& value) {
    int64_t firstIndex = faceIndex;
    int64_t numFaces = 0;

    switch (primitiveMode) {
    case MeshPrimitive::Mode::TRIANGLE_STRIP:
      numFaces = value.size() - 2;
      break;
    case MeshPrimitive::Mode::TRIANGLE_FAN:
      numFaces = value.size() - 2;
      firstIndex++;
      break;
    case MeshPrimitive::Mode::TRIANGLES:
      numFaces = value.size() / 3;
      firstIndex *= 3;
      break;
    default:
      // Unsupported primitive mode.
      return {-1, -1, -1};
    }

    if (faceIndex < 0 || faceIndex >= numFaces) {
      return {-1, -1, -1};
    }

    std::array<int64_t, 3> result;

    if (primitiveMode == MeshPrimitive::Mode::TRIANGLE_FAN) {
      result[0] = value[0];
      result[1] = firstIndex < value.size() ? value[firstIndex] : -1;
      result[2] = firstIndex + 1 < value.size() ? value[firstIndex + 1] : -1;
    } else {
      for (int64_t i = 0; i < 3; i++) {
        int64_t index = firstIndex + i;
        result[i] = index < value.size() ? value[index] : -1;
      }
    }

    return result;
  }

  int64_t faceIndex;
  int64_t vertexCount;
  int32_t primitiveMode;
}; // namespace CesiumGltf

/**
 * Visitor that retrieves the vertex index from the given accessor type as an
 * int64_t. This should be initialized with the index (within the
 * accessor itself) of the vertex index.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct IndexFromAccessor {
  int64_t operator()(std::monostate) { return -1; }

  template <typename T>
  int64_t operator()(const CesiumGltf::AccessorView<T>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }

    return value[index];
  }

  int64_t index;
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
 * Retrieves an accessor view for the specified texture coordinate set from
 * the given glTF primitive and model. This verifies that the accessor is of a
 * valid type. If not, the returned accessor view will be invalid.,
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

/**
 * Type definition for quaternion accessors, as used in ExtMeshGpuInstancing
 * rotations and animation samplers.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC4<uint8_t>>,
    AccessorView<AccessorTypes::VEC4<int8_t>>,
    AccessorView<AccessorTypes::VEC4<uint16_t>>,
    AccessorView<AccessorTypes::VEC4<int16_t>>,
    AccessorView<AccessorTypes::VEC4<float>>>
    QuaternionAccessorType;

QuaternionAccessorType
getQuaternionAccessorView(const Model& model, const Accessor* accessor);

QuaternionAccessorType
getQuaternionAccessorView(const Model& model, int32_t accessorIndex);
} // namespace CesiumGltf
