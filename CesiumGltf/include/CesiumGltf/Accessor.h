#pragma once

#include "CesiumGltf/AccessorSpec.h"
#include "CesiumGltf/Library.h"

#include <cstdint>

// forward declarations
namespace CesiumGltf {
struct Model;
}

namespace CesiumGltf {

/** @copydoc AccessorSpec */
struct CESIUMGLTF_API Accessor final : public AccessorSpec {
  /**
   * @brief Computes the number of components for a given accessor type.
   *
   * For example `CesiumGltf::Accessor::Type::SCALAR` has 1 component while
   * `CesiumGltf::Accessor::Type::VEC4` has 4 components.
   *
   * @param type The accessor type.
   * @return The number of components. Returns 0 if {@link Accessor::type} is
   * not a valid enumeration value.
   */
  static int8_t computeNumberOfComponents(const std::string& type) noexcept;

  /**
   * @brief Computes the number of bytes for a given accessor component type.
   *
   * For example `CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT` is 2
   * bytes while `CesiumGltf::Accessor::ComponentType::FLOAT` is 4 bytes.
   *
   * @param componentType The accessor component type.
   * @return The number of bytes for the component type. Returns 0 if
   * {@link Accessor::componentType} is not a valid enumeration value.
   */
  static int8_t
  computeByteSizeOfComponent(const int32_t componentType) noexcept;

  /**
   * @brief Computes the number of bytes in each element for a given accessor
   * type and accessor component type.
   *
   * This is computed by multiplying
   * {@link Accessor::computeByteSizeOfComponent} by
   * {@link Accessor::computeNumberOfComponents}.
   *
   * @param type The accessor type.
   * @param componentType The accessor component type.
   * @return The number of bytes in each element. Returns 0 if
   * {@link Accessor::type} or {@link Accessor::componentType}
   * does not have a valid enumeration value.
   */
  static int64_t computeByteSizeOfElement(
      const std::string& type,
      const int32_t componentType) noexcept;

  /**
   * @brief Computes the number of bytes in the accessor for a given accessor
   * type, accessor component type, and accessor count.
   *
   * This is computed by multiplying
   * {@link Accessor::computeByteSizeOfElement} by
   * {@link Accessor::count}.
   *
   * @param type The accessor type.
   * @param componentType The accessor component type.
   * @param count The accessor count.
   * @return The number of bytes in the accessor. Returns 0 if
   * {@link Accessor::type} or {@link Accessor::componentType} does not have a
   * valid enumeration value.
   */
  static int64_t computeByteSize(
      const std::string& type,
      const int32_t componentType,
      const int64_t count) noexcept;

  static int32_t computeIndexComponentType(const int64_t vertexCount);

  /**
   * @brief Computes the number of components for this accessor.
   *
   * For example if this accessor's {@link Accessor::type} is
   * `CesiumGltf::Accessor::Type::SCALAR`, then it has 1 component, while if
   * it's `CesiumGltf::Accessor::Type::VEC4` it has 4 components.
   *
   * @return The number of components in this accessor. Returns 0 if this
   * accessor's {@link Accessor::type} does not have a valid enumeration value.
   */
  int8_t computeNumberOfComponents() const noexcept;

  /**
   * @brief Computes the number of bytes for this accessor's component type.
   *
   * For example if this accessor's {@link Accessor::componentType} is
   * `CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT`, then the component
   * type is 2 bytes, while if it's `CesiumGltf::Accessor::ComponentType::FLOAT`
   * then it is 4 bytes.
   *
   * @return The number of bytes for this accessor's component type. Returns 0
   * if this accessor's {@link Accessor::componentType} does not have a valid
   * enumeration value.
   */
  int8_t computeByteSizeOfComponent() const noexcept;

  /**
   * @brief Computes the number of bytes for each element in this accessor.
   *
   * This is computed by multiplying
   * {@link Accessor::computeByteSizeOfComponent} by
   * {@link Accessor::computeNumberOfComponents}.
   *
   * @return The total number of bytes for each element. Returns 0 if this
   * accessor's {@link Accessor::type} or {@link Accessor::componentType} does
   * not have a valid enumeration value.
   */
  int64_t computeByteSizeOfElement() const noexcept;

  /**
   * @brief Computes the total number of bytes for this accessor.
   *
   * This is computed by multiplying
   * {@link Accessor::computeByteSizeOfElement} by
   * {@link Accessor::count}.
   *
   * @return The total number of bytes for this accessor. Returns 0 if this
   * accessor's {@link Accessor::type} or {@link Accessor::componentType} does
   * not have a valid enumeration value.
   */
  int64_t computeByteSize() const noexcept;

  /**
   * @brief Computes this accessor's stride.
   *
   * The stride is the number of bytes between the same elements of successive
   * vertices. The returned value will be at least as large as
   * {@link Accessor::computeByteSizeOfElement}, but maybe be larger if this
   * accessor's data is interleaved with other accessors.
   *
   * The behavior is undefined if this accessor is not part of the given model.
   *
   * @param model The model that this accessor is a part of.
   * @return The stride in bytes. Returns 0 if this accessor's
   * {@link Accessor::type} or {@link Accessor::componentType} does not have
   * a valid enumeration value, or if {@link Accessor::bufferView} does not
   * refer to a valid {@link BufferView}.
   */
  int64_t computeByteStride(const CesiumGltf::Model& model) const noexcept;
};
} // namespace CesiumGltf
