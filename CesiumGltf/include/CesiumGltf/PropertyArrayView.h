#pragma once

#include "CesiumGltf/PropertyType.h"
#include "getOffsetFromOffsetsBuffer.h"

#include <CesiumUtility/SpanHelper.h>

#include <gsl/span>

#include <cassert>
#include <cstddef>
#include <variant>
#include <vector>

namespace CesiumGltf {
/**
 * @brief A view on an array element of a {@link PropertyTableProperty}
 * or {@link PropertyTextureProperty}.
 *
 * Provides utility to retrieve the data stored in the array of
 * elements via the array index operator.
 */
template <typename ElementType> class PropertyArrayView {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView() : _values{} {}

  /**
   * @brief Constructs an array view from a buffer.
   *
   * @param buffer The buffer containing the values.
   */
  PropertyArrayView(const gsl::span<const std::byte>& buffer) noexcept
      : _values{CesiumUtility::reintepretCastSpan<const ElementType>(buffer)} {}

  /**
   * @brief Constructs an array view from a vector of values. This is mainly
   * used when the values cannot be viewed in place.
   *
   * @param values The vector containing the values.
   */
  PropertyArrayView(const std::vector<ElementType>&& values)
      : _values{std::move(values)} {}

  const ElementType& operator[](int64_t index) const noexcept {
    return std::visit(
        [index](auto const& values) -> auto const& { return values[index]; },
        _values);
  }

  int64_t size() const noexcept {
    return std::visit(
        [](auto const& values) { return static_cast<int64_t>(values.size()); },
        _values);
  }

private:
  using ArrayType =
      std::variant<gsl::span<const ElementType>, std::vector<ElementType>>;
  ArrayType _values;
};

template <> class PropertyArrayView<bool> {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView() : _values{}, _size{0} {}

  /**
   * @brief Constructs an array view from a buffer.
   *
   * @param buffer The buffer containing the values.
   * @param bitOffset The offset into the buffer where the values actually
   * begin.
   * @param size The number of values in the array.
   */
  PropertyArrayView(
      const gsl::span<const std::byte>& buffer,
      int64_t bitOffset,
      int64_t size) noexcept
      : _values{BooleanArrayView{buffer, bitOffset}}, _size{size} {}

  /**
   * @brief Constructs an array view from a vector of values. This is mainly
   * used when the values cannot be viewed in place.
   *
   * @param values The vector containing the values.
   */
  PropertyArrayView(const std::vector<bool>&& values)
      : _values{std::move(values)}, _size{0} {
    size_t size = std::get<std::vector<bool>>(_values).size();
    _size = static_cast<int64_t>(size);
  }

  bool operator[](int64_t index) const noexcept {
    return std::visit(
        [index](auto const& values) -> auto const { return values[index]; },
        _values);
  }

  int64_t size() const noexcept { return _size; }

private:
  struct BooleanArrayView {
    gsl::span<const std::byte> values;
    int64_t bitOffset = 0;

    bool operator[](int64_t index) const noexcept {
      index += bitOffset;
      const int64_t byteIndex = index / 8;
      const int64_t bitIndex = index % 8;
      const int bitValue = static_cast<int>(values[byteIndex] >> bitIndex) & 1;
      return bitValue == 1;
    }
  };

  using ArrayType = std::variant<BooleanArrayView, std::vector<bool>>;
  ArrayType _values;
  int64_t _size;
};

template <> class PropertyArrayView<std::string_view> {
public:
  /**
   * @brief Constructs an empty array view.
   */
  PropertyArrayView() : _values{}, _size{0} {}

  /**
   * @brief Constructs an array view from buffers and their information.
   *
   * @param values The buffer containing the values.
   * @param stringOffsets The buffer containing the string offsets.
   * @param stringOffsetType The component type of the string offsets.
   * @param size The number of values in the array.
   */
  PropertyArrayView(
      const gsl::span<const std::byte>& values,
      const gsl::span<const std::byte>& stringOffsets,
      PropertyComponentType stringOffsetType,
      int64_t size) noexcept
      : _values{StringArrayView{values, stringOffsets, stringOffsetType}},
        _size{size} {}

  /**
   * @brief Constructs an array view from a vector of values. This is mainly
   * used when the values cannot be viewed in place.
   *
   * @param values The vector containing the values.
   */
  PropertyArrayView(const std::vector<std::string>&& values) noexcept
      : _values{std::move(values)}, _size{0} {
    size_t size = std::get<std::vector<std::string>>(_values).size();
    _size = static_cast<int64_t>(size);
  }

  std::string_view operator[](int64_t index) const noexcept {
    return std::visit(
        [index](auto const& values) -> auto const {
          return std::string_view(values[index]);
        },
        _values);
  }

  int64_t size() const noexcept { return _size; }

private:
  struct StringArrayView {
    gsl::span<const std::byte> values;
    gsl::span<const std::byte> stringOffsets;
    PropertyComponentType stringOffsetType = PropertyComponentType::None;

    std::string_view operator[](int64_t index) const noexcept {
      const size_t currentOffset =
          getOffsetFromOffsetsBuffer(index, stringOffsets, stringOffsetType);
      const size_t nextOffset = getOffsetFromOffsetsBuffer(
          index + 1,
          stringOffsets,
          stringOffsetType);
      return std::string_view(
          reinterpret_cast<const char*>(values.data() + currentOffset),
          (nextOffset - currentOffset));
    }
  };

  using ArrayType = std::variant<StringArrayView, std::vector<std::string>>;
  ArrayType _values;
  int64_t _size;
};
} // namespace CesiumGltf
