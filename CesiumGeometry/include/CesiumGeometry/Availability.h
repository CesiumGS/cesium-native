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
uint8_t countOnesInByte(uint8_t _byte);
uint32_t countOnesInBuffer(gsl::span<const std::byte> buffer);
} // namespace AvailabilityUtilities

struct CESIUMGEOMETRY_API ConstantAvailability {
  bool constant;
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

/**
 * @brief Availability nodes wrap subtree objects and link them together to
 * form a downwardly traversable availability tree.
 */
struct CESIUMGEOMETRY_API AvailabilityNode {
  /**
   * @brief The subtree data for this node.
   *
   * If a node exists but its subtree does not exist, it indicates that the
   * subtree is known to be available and is actively in the process of loading.
   */
  std::optional<AvailabilitySubtree> subtree;

  /**
   * @brief The child nodes for this subtree node.
   */
  std::vector<std::unique_ptr<AvailabilityNode>> childNodes;

  /**
   * @brief Creates an empty instance;
   */
  AvailabilityNode() noexcept;

  /**
   * @brief Sets the loaded subtree for this availability node.
   *
   * @param subtree_ The loaded subtree to set for this node.
   * @param maxChildrenSubtrees The maximum number of children this subtree
   * could possible have if all of them happen to be available.
   */
  void setLoadedSubtree(
      AvailabilitySubtree&& subtree_,
      uint32_t maxChildrenSubtrees) noexcept;
};

struct CESIUMGEOMETRY_API AvailabilityTree {
  std::unique_ptr<AvailabilityNode> pRoot;
};

class CESIUMGEOMETRY_API AvailabilityAccessor {
public:
  AvailabilityAccessor(
      const AvailabilityView& view,
      const AvailabilitySubtree& subtree) noexcept;

  bool isBufferView() const noexcept {
    return pBufferView != nullptr && bufferAccessor;
  }

  bool isConstant() const noexcept { return pConstant != nullptr; }

  /**
   * @brief Unsafe if isConstant is false.
   */
  bool getConstant() const { return pConstant->constant; }

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
