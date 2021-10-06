#pragma once

#include "Library.h"

#include <gsl/span>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace CesiumGeometry {

namespace AvailabilityUtilities {
constexpr uint8_t countOnesInByte(uint8_t _byte);

constexpr uint64_t countOnesInBuffer(gsl::span<const std::byte> buffer);
} // namespace AvailabilityUtilities

struct CESIUMGEOMETRY_API ConstantAvailability {
  uint8_t constant;
};

struct CESIUMGEOMETRY_API SubtreeBufferView {
  uint32_t byteOffset;
  uint32_t byteLength;
  uint8_t buffer;
};

typedef std::variant<ConstantAvailability, SubtreeBufferView> AvailabilityView;

struct CESIUMGEOMETRY_API AvailabilitySubtree {
  AvailabilityView tileAvailability;
  AvailabilityView contentAvailability;
  AvailabilityView subtreeAvailability;
  std::vector<std::vector<std::byte>> buffers;
};

struct CESIUMGEOMETRY_API AvailabilityNode {
  AvailabilitySubtree subtree;
  std::vector<std::unique_ptr<AvailabilityNode>> childNodes;

  AvailabilityNode(
      AvailabilitySubtree&& subtree_,
      uint32_t maxChildrenSubtrees) noexcept;
};

struct CESIUMGEOMETRY_API AvailabilityTree {
  std::unique_ptr<AvailabilityNode> pRoot;
};

class CESIUMGEOMETRY_API AvailabilityAccessor {
public:
  AvailabilityAccessor(
      const AvailabilityView view,
      const AvailabilitySubtree& subtree) noexcept;

  bool isBufferView() const noexcept {
    return pBufferView != nullptr && bufferAccessor;
  }

  bool isConstant() const noexcept { return pConstant != nullptr; }

  /**
   * @brief Unsafe if isConstant is false.
   */
  uint8_t getConstant() const { return pConstant->constant; }

  /**
   * @brief Unsafe is isBufferView is false.
   */
  const gsl::span<const std::byte>& getBufferAccessor() const {
    return *bufferAccessor;
  }

  /**
   * @brief Unsafe if isBufferView is false.
   */
  const std::byte& operator[](size_t i) const {
    return bufferAccessor.value()[i];
  }

  /**
   * @brief Unsafe if isBufferView is false;
   */
  size_t size() const { return pBufferView->byteLength; }

private:
  const SubtreeBufferView* pBufferView;
  const ConstantAvailability* pConstant;
  std::optional<gsl::span<const std::byte>> bufferAccessor;
};
} // namespace CesiumGeometry