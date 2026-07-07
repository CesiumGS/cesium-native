#pragma once

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/MeshPrimitive.h>

#include <glm/common.hpp>
#include <glm/ext/quaternion_double.hpp>

#include <array>
#include <type_traits>
#include <variant>

namespace CesiumGltf {

namespace CesiumImpl {
template <typename T> double denormalize(T value) {
  if constexpr (std::is_floating_point_v<T>) {
    return double(value);
  } else if constexpr (std::is_signed_v<T>) {
    return glm::max(double(value) / std::numeric_limits<T>::max(), -1.0);
  } else {
    return double(value) / std::numeric_limits<T>::max();
  }
}
} // namespace CesiumImpl

/**
 * @brief Visitor that retrieves the count of elements in the given accessor
 * type as an int64_t.
 */
struct CountFromAccessor {
  /** @brief Attempts to obtain an element count from an empty accessor variant,
   * resulting in 0. */
  int64_t operator()(std::monostate) { return 0; }

  /** @brief Attempts to obtain an element count from an \ref AccessorView. */
  template <typename T> int64_t operator()(const AccessorView<T>& value) {
    return value.size();
  }
};

/**
 * @brief Visitor that retrieves the status from the given accessor. Returns an
 * invalid status for a std::monostate (interpreted as a nonexistent accessor).
 */
struct StatusFromAccessor {
  /** @brief Attempts to obtain an \ref AccessorViewStatus from an empty
   * accessor variant, resulting in \ref
   * AccessorViewStatus::InvalidAccessorIndex. */
  AccessorViewStatus operator()(std::monostate) {
    return AccessorViewStatus::InvalidAccessorIndex;
  }

  /** @brief Attempts to obtain an \ref AccessorViewStatus from an \ref
   * AccessorView. */
  template <typename T>
  AccessorViewStatus operator()(const AccessorView<T>& value) {
    return value.status();
  }
};

/**
 * @brief Type definition for all kinds of position (`POSITION`) accessors.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC3<int8_t>>,
    AccessorView<AccessorTypes::VEC3<uint8_t>>,
    AccessorView<AccessorTypes::VEC3<int16_t>>,
    AccessorView<AccessorTypes::VEC3<uint16_t>>,
    AccessorView<AccessorTypes::VEC3<float>>>
    PositionAccessorType;

/**
 * @brief Retrieves an accessor view for the position attribute from the given
 * glTF primitive and model. This verifies that the accessor is of a valid type.
 * If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param primitive The primitive.
 *
 * @returns A position accessor view.
 */
PositionAccessorType
getPositionAccessorView(const Model& model, const MeshPrimitive& primitive);

/**
 * @brief Visitor that retrieves the position from the given accessor type
 * as a `glm::dvec3`.
 *
 * There are technically no invalid position values, so `std::nullopt` is used
 * to denote an erroneous value.
 */
struct PositionFromAccessor {
  /**
   * @brief Attempts to obtain a `glm::dvec3` at the given index from an
   * accessor over a vec3. The values will be cast to `double` and, if
   * applicable, normalized based on `std::numeric_limits<T>::max()`. If the
   * index is invalid, `std::nullopt` is returned instead.
   */
  template <typename T>
  std::optional<glm::dvec3>
  operator()(const AccessorView<AccessorTypes::VEC3<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      return glm::dvec3(
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]),
          CesiumImpl::denormalize(value[index].value[2]));
    } else {
      return glm::dvec3(
          value[index].value[0],
          value[index].value[1],
          value[index].value[2]);
    }
  }

  /** @brief The index of the position to obtain. */
  int64_t index;
};

/**
 * @brief Type definition for all kinds of normal (`NORMAL`) accessors.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC3<int8_t>>,
    AccessorView<AccessorTypes::VEC3<int16_t>>,
    AccessorView<AccessorTypes::VEC3<float>>>
    NormalAccessorType;

/**
 * @brief Retrieves an accessor view for the normal attribute from the given
 * glTF primitive and model. This verifies that the accessor is of a valid type.
 * If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param primitive The primitive.
 *
 * @returns A normal accessor view.
 */
NormalAccessorType
getNormalAccessorView(const Model& model, const MeshPrimitive& primitive);

/**
 * @brief Visitor that retrieves the normal from the given accessor type
 * as a `glm::dvec3`.
 *
 * There are technically no invalid normal values, so `std::nullopt` is used to
 * denote an erroneous value.
 */
struct NormalFromAccessor {
  /**
   * @brief Attempts to obtain a `glm::dvec3` at the given index from an
   * accessor over a vec3. The values will be cast to `double` and, if
   * applicable, normalized based on `std::numeric_limits<T>::max()`. If the
   * index is invalid, `std::nullopt` is returned instead.
   */
  template <typename T>
  std::optional<glm::dvec3>
  operator()(const AccessorView<AccessorTypes::VEC3<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      return glm::dvec3(
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]),
          CesiumImpl::denormalize(value[index].value[2]));
    } else {
      return glm::dvec3(
          value[index].value[0],
          value[index].value[1],
          value[index].value[2]);
    }
  }

  /** @brief The index of the normal to obtain. */
  int64_t index;
};

/**
 * @brief Type definition for all kinds of feature ID (`_FEATURE_ID_n`)
 * attribute accessors.
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
 * @brief Retrieves an accessor view for the specified feature ID attribute from
 * the given glTF primitive and model. This verifies that the accessor is of a
 * valid type. If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param primitive The primitive.
 * @param featureIdSetIndex The set index of the attribute, i.e. `n` for
 * `_FEATURE_ID_n`.
 *
 * @returns A feature ID accessor view.
 */
FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t featureIdSetIndex);

/**
 * @brief Retrieves an accessor view for the specified feature ID attribute from
 * the given glTF node and model, if the node contains an
 * EXT_mesh_gpu_instancing property. This verifies that the accessor is of a
 * valid type. If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param node The node.
 * @param featureIdSetIndex The set index of the attribute, i.e. `n` for
 * `_FEATURE_ID_n`.
 *
 * @returns A feature ID accessor view.
 */
FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const Node& node,
    int32_t featureIdSetIndex);

/**
 * @brief Visitor that retrieves the feature ID from the given accessor type as
 * an int64_t. This should be initialized with the index of the vertex whose
 * feature ID is being queried.
 *
 * -1 is used to indicate errors retrieving the feature ID, e.g., if the given
 * index was out-of-bounds.
 */
struct FeatureIdFromAccessor {
  /** @brief Attempts to obtain a feature ID from an \ref AccessorView over
   * float values, returning the float value rounded to the nearest `int64_t`.
   */
  int64_t operator()(const AccessorView<float>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64_t>(glm::round(value[index]));
  }

  /** @brief Attempts to obtain a feature ID from an \ref AccessorView. */
  template <typename T> int64_t operator()(const AccessorView<T>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64_t>(value[index]);
  }

  /** @brief The index of the vertex whose feature ID is being queried. */
  int64_t index;
};

/**
 * @brief Type definition for all kinds of index accessors. std::monostate
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
 * @brief Retrieves an accessor view for the indices of the given glTF primitive
 * from the model. The primitive may not specify any indices; if so,
 * std::monostate is returned.
 *
 * @param model The model.
 * @param primitive The primitive.
 *
 * @returns An index accessor view.
 */
IndexAccessorType
getIndexAccessorView(const Model& model, const MeshPrimitive& primitive);

/**
 * @brief Retrieves an indices accessor view of the accessor at the given index.
 *
 * @param model The model.
 * @param index The accessor index.
 *
 * @returns An index accessor view.
 */
IndexAccessorType getIndexAccessorView(const Model& model, int32_t index);

/**
 * @brief Retrieves an indices accessor view of given accessor.
 */
IndexAccessorType
getIndexAccessorView(const Model& model, const Accessor& accessor);

/**
 * @brief Visitor that returns the number of indices contained in an
 * IndexAccessorType variant.
 */
struct NumIndicesFromAccessor {
  /**
   * @brief Attempts to obtain a number of indices from an empty
   * IndexAccessorType, resulting in 0.
   */
  int64_t operator()(std::monostate) { return 0; }

  /**
   * @brief Attempts to obtain a number of indices from an \ref AccessorView.
   */
  template <typename T> int64_t operator()(const AccessorView<T>& value) {
    return value.size();
  }
};

/**
 * @brief Returns the maximum possible index value for the given
 * IndexAccessorType.
 */
struct MaxIndexValueFromAccessor {
  /**
   * @brief Attempts to obtain a maximum index value from an empty
   * IndexAccessorType, resulting in -1.
   */
  int64_t operator()(std::monostate) { return -1; }

  /**
   * @brief Attempts to obtain a maximum index value from an \ref AccessorView.
   */
  template <typename T> int64_t operator()(const AccessorView<T>& /*value*/) {
    return static_cast<int64_t>(std::numeric_limits<T>::max());
  }
};

/**
 * @brief Visitor that retrieves the vertex indices from the given accessor type
 * corresponding to a given face index. These indices are returned as an array
 * of int64_ts. This should be initialized with the index of the face, the
 * total number of vertices in the primitive, and the
 * `CesiumGltf::MeshPrimitive::Mode` of the primitive.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct IndicesForFaceFromAccessor {
  /** @brief Attempts to obtain the indices for the given face from an empty
   * accessor variant, using the \ref vertexCount property. */
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
        result[static_cast<size_t>(i)] =
            vertexIndex < vertexCount ? vertexIndex : -1;
      }
    }

    return result;
  }

  /** @brief Attempts to obtain the indices for the given face from an \ref
   * AccessorView, using the view's size and contents rather than the \ref
   * vertexCount property. */
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
      result[1] = firstIndex < value.size()
                      ? static_cast<int64_t>(value[firstIndex])
                      : -1;
      result[2] = firstIndex + 1 < value.size()
                      ? static_cast<int64_t>(value[firstIndex + 1])
                      : -1;
    } else {
      for (int64_t i = 0; i < 3; i++) {
        int64_t index = firstIndex + i;
        result[static_cast<size_t>(i)] =
            index < value.size() ? static_cast<int64_t>(value[index]) : -1;
      }
    }

    return result;
  }

  /** @brief The index of the face to obtain indices for. */
  int64_t faceIndex;
  /** @brief The total number of vertices in the data being accessed. */
  int64_t vertexCount;
  /** @brief The \ref MeshPrimitive::Mode of the data being accessed. */
  int32_t primitiveMode;
}; // namespace CesiumGltf

/**
 * @brief Visitor that retrieves the vertex index from the given accessor type
 * as an int64_t. This should be initialized with the index (within the accessor
 * itself) of the vertex index.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct IndexFromAccessor {
  /** @brief Attempts to obtain a vertex index from an empty \ref
   * IndexAccessorType, resulting in -1. */
  int64_t operator()(std::monostate) { return -1; }

  /** @brief Attempts to obtain a vertex index from an \ref
   * CesiumGltf::AccessorView. */
  template <typename T>
  int64_t operator()(const CesiumGltf::AccessorView<T>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }

    return value[index];
  }

  /** @brief The index of the vertex index within the accessor itself. */
  int64_t index;
};

/**
 * @brief Type definition for all kinds of texture coordinate (`TEXCOORD_n`)
 * accessors.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC2<int8_t>>,
    AccessorView<AccessorTypes::VEC2<uint8_t>>,
    AccessorView<AccessorTypes::VEC2<int16_t>>,
    AccessorView<AccessorTypes::VEC2<uint16_t>>,
    AccessorView<AccessorTypes::VEC2<float>>>
    TexCoordAccessorType;

/**
 * @brief Retrieves an accessor view for the specified texture coordinate set
 * from the given glTF primitive and model. This verifies that the accessor is
 * of a valid type. If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param primitive The primitive.
 * @param textureCoordinateSetIndex The set index of the attribute, i.e. `n` for
 * `TEXCOORD_n`.
 *
 * @returns A texture coordinate accessor view.
 */
TexCoordAccessorType getTexCoordAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t textureCoordinateSetIndex);

/**
 * @brief Visitor that retrieves the texture coordinates from the given accessor
 * type as a `glm::dvec2`. This should be initialized with the target index.
 *
 * There are technically no invalid UV values because of clamp / wrap
 * behavior, so `std::nullopt` is used to denote an erroneous value.
 */
struct TexCoordFromAccessor {
  /**
   * @brief Attempts to obtain a `glm::dvec2` at the given index from an
   * accessor over a vec2. The values will be cast to `double` and normalized
   * based on `std::numeric_limits<T>::max()`. If the index is invalid,
   * `std::nullopt` is returned instead.
   */
  template <typename T>
  std::optional<glm::dvec2>
  operator()(const AccessorView<AccessorTypes::VEC2<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      return glm::dvec2(
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]));
    } else {
      return glm::dvec2(value[index].value[0], value[index].value[1]);
    }
  }

  /** @brief The index of texcoords to obtain. */
  int64_t index;
};

/**
 * @brief Type definition for quaternion accessors, as used in
 * ExtMeshGpuInstancing rotations and animation samplers.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC4<int8_t>>,
    AccessorView<AccessorTypes::VEC4<uint8_t>>,
    AccessorView<AccessorTypes::VEC4<int16_t>>,
    AccessorView<AccessorTypes::VEC4<uint16_t>>,
    AccessorView<AccessorTypes::VEC4<float>>>
    QuaternionAccessorType;

/**
 * @brief Obtains a \ref QuaternionAccessorType from the given \ref Accessor on
 * the given \ref Model.
 *
 * @param model The model containing the quaternion accessor.
 * @param accessor An accessor from which the quaternion will be obtained.
 * @returns A quaternion accessor view from the data in `accessor`. If no
 * such view could be obtained, the default value for \ref
 * QuaternionAccessorType will be returned instead.
 */
QuaternionAccessorType
getQuaternionAccessorView(const Model& model, const Accessor& accessor);

/**
 * @brief Obtains a \ref QuaternionAccessorType from the given accessor index on
 * the given \ref Model.
 *
 * @param model The model containing the quaternion accessor.
 * @param accessorIndex An index to the accessor from which the quaternion will
 * be obtained.
 * @returns A quaternion accessor view from the data in the accessor at
 * `accessorIndex`. If no such view could be obtained, the default value for
 * \ref QuaternionAccessorType will be returned instead.
 */
QuaternionAccessorType
getQuaternionAccessorView(const Model& model, int32_t accessorIndex);

/**
 * @brief Visitor that retrieves the quaternion from the given accessor type
 * as a glm::dquat.
 */
struct QuaternionFromAccessor {
  /**
   * @brief Attempts to obtain a `glm::dquat` at the given index from an
   * accessor over a vec4. The values will be cast to `double` and, if
   * applicable, normalized based on `std::numeric_limits<T>::max()`. If the
   * index is invalid, `std::nullopt` is returned instead.
   */
  template <typename T>
  std::optional<glm::dquat>
  operator()(const AccessorView<AccessorTypes::VEC4<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      // glm quat constructor is w,x,y,z
      return glm::dquat(
          CesiumImpl::denormalize(value[index].value[3]),
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]),
          CesiumImpl::denormalize(value[index].value[2]));
    } else {
      // glm quat constructor is w,x,y,z
      return glm::dquat(
          value[index].value[3],
          value[index].value[0],
          value[index].value[1],
          value[index].value[2]);
    }
  }

  /** @brief The index of the quaternion to obtain. */
  int64_t index;
};

/**
 * @brief Type definition for all kinds of color (`COLOR_n`) accessors.
 */
typedef std::variant<
    AccessorView<AccessorTypes::VEC3<uint8_t>>,
    AccessorView<AccessorTypes::VEC3<uint16_t>>,
    AccessorView<AccessorTypes::VEC3<float>>,
    AccessorView<AccessorTypes::VEC4<uint8_t>>,
    AccessorView<AccessorTypes::VEC4<uint16_t>>,
    AccessorView<AccessorTypes::VEC4<float>>>
    ColorAccessorType;

/**
 * @brief Retrieves an accessor view for the specified color attribute
 * from the given glTF primitive and model. This verifies that the accessor is
 * of a valid type. If not, the returned accessor view will be invalid.
 *
 * @param model The model.
 * @param primitive The primitive.
 * @param colorSetIndex The set index of the attribute, i.e. `n` for `COLOR_n`.
 *
 * @returns A color accessor view.
 */
ColorAccessorType getColorAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t colorSetIndex);

/**
 * @brief Visitor that retrieves the color from the given accessor type
 * as a `glm::dvec4`.
 */
struct ColorFromAccessor {
  /**
   * @brief Attempts to obtain a `glm::dvec4` at the given index from an
   * accessor over a vec3. The values will be cast to `double` and, if
   * applicable, normalized based on `std::numeric_limits<T>::max()`. The fourth
   * component will be set to 1.0. If the index is invalid, `std::nullopt` is
   * returned instead.
   */
  template <typename T>
  std::optional<glm::dvec4>
  operator()(const AccessorView<AccessorTypes::VEC3<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      return glm::dvec4(
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]),
          CesiumImpl::denormalize(value[index].value[2]),
          1.0);
    } else {
      return glm::dvec4(
          value[index].value[0],
          value[index].value[1],
          value[index].value[2],
          1.0);
    }
  }

  /**
   * @brief Attempts to obtain a `glm::dvec4` at the given index from an
   * accessor over a vec4. The values will be cast to `double` and, if
   * applicable, normalized based on `std::numeric_limits<T>::max()`. If the
   * index is invalid, `std::nullopt` is returned instead.
   */
  template <typename T>
  std::optional<glm::dvec4>
  operator()(const AccessorView<AccessorTypes::VEC4<T>>& value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    if (value.normalized()) {
      return glm::dvec4(
          CesiumImpl::denormalize(value[index].value[0]),
          CesiumImpl::denormalize(value[index].value[1]),
          CesiumImpl::denormalize(value[index].value[2]),
          CesiumImpl::denormalize(value[index].value[3]));
    } else {
      return glm::dvec4(
          value[index].value[0],
          value[index].value[1],
          value[index].value[2],
          value[index].value[3]);
    }
  }

  /** @brief The index of the color to obtain. */
  int64_t index;
};

} // namespace CesiumGltf
