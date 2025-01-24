#pragma once

#include <CesiumGeometry/Library.h>
#include <CesiumUtility/Assert.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace CesiumGeometry {

namespace AvailabilityUtilities {
uint8_t countOnesInByte(uint8_t _byte);
uint32_t countOnesInBuffer(std::span<const std::byte> buffer);
} // namespace AvailabilityUtilities

/**
 * @brief An availability value that is a constant boolean value.
 */
struct CESIUMGEOMETRY_API ConstantAvailability {
  /**
   * @brief The constant value.
   */
  bool constant;
};

/**
 * @brief An availability value that needs to be obtained using an offset into a
 * buffer.
 */
struct CESIUMGEOMETRY_API SubtreeBufferView {
  /**
   * @brief The offset into the buffer to read from.
   */
  uint32_t byteOffset;
  /**
   * @brief The number of bytes after the offset to read until.
   */
  uint32_t byteLength;
  /**
   * @brief The index into \ref AvailabilitySubtree::buffers that this \ref
   * SubtreeBufferView corresponds to.
   */
  uint8_t buffer;
};

/**
 * @brief A view into availability information for part of the availability
 * tree. This could be either a constant boolean value or a descriptor pointing
 * to a buffer in an \ref AvailabilitySubtree where the information will be
 * looked up.
 *
 * Instead of using this type directly, \ref AvailabilityAccessor can be used to
 * work with it safely.
 */
typedef std::variant<ConstantAvailability, SubtreeBufferView> AvailabilityView;

/**
 * @brief The subtree data for an \ref AvailabilityNode, containing information
 * on tile, content, and subtree availability.
 */
struct CESIUMGEOMETRY_API AvailabilitySubtree {
  /**
   * @brief The availability information corresponding to \ref
   * TileAvailabilityFlags::TILE_AVAILABLE.
   */
  AvailabilityView tileAvailability;
  /**
   * @brief The availability information corresponding to \ref
   * TileAvailabilityFlags::CONTENT_AVAILABLE.
   */
  AvailabilityView contentAvailability;
  /**
   * @brief The availability information corresponding to \ref
   * TileAvailabilityFlags::SUBTREE_AVAILABLE and \ref
   * TileAvailabilityFlags::SUBTREE_LOADED.
   */
  AvailabilityView subtreeAvailability;
  /**
   * @brief Subtree buffers that may be referenced by a \ref SubtreeBufferView.
   */
  std::vector<std::vector<std::byte>> buffers;
};

/**
 * @brief Availability nodes wrap \ref AvailabilitySubtree objects and link them
 * together to form a downwardly traversable availability tree.
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

/**
 * @brief A downwardly-traversable tree of \ref AvailabilityNode objects.
 */
struct CESIUMGEOMETRY_API AvailabilityTree {
  /**
   * @brief The root \ref AvailabilityNode of this tree.
   */
  std::unique_ptr<AvailabilityNode> pRoot;
};

/**
 * @brief Accessor for use with \ref AvailabilityView in order to safely obtain
 * the contents of the view.
 */
class CESIUMGEOMETRY_API AvailabilityAccessor {
public:
  /**
   * @brief Creates a new AvailabilityAccessor.
   *
   * @param view The view whose contents will be accessed by this accessor.
   * @param subtree The subtree that corresponds to the view.
   */
  AvailabilityAccessor(
      const AvailabilityView& view,
      const AvailabilitySubtree& subtree) noexcept;

  /**
   * @brief Is this \ref AvailabilityAccessor accessing a \ref
   * SubtreeBufferView?
   *
   * @returns True if the \ref AvailabilityView is a \ref SubtreeBufferView with
   * a valid index, offset, and length, or false otherwise.
   */
  bool isBufferView() const noexcept {
    return pBufferView != nullptr && bufferAccessor;
  }

  /**
   * @brief Is this \ref AvailabilityAccessor accessing a \ref
   * ConstantAvailability?
   *
   * @returns True if the \ref AvailabilityView is a \ref ConstantAvailability,
   * false otherwise.
   */
  bool isConstant() const noexcept { return pConstant != nullptr; }

  /**
   * @brief Obtains the constant value of the \ref AvailabilityView.
   *
   * @warning Unsafe to use if isConstant is false.
   * @returns The constant value.
   */
  bool getConstant() const { return pConstant->constant; }

  /**
   * @brief Obtains an accessor to the buffer used by the \ref AvailabilityView.
   *
   * @warning Unsafe to use if isBufferView is false.
   * @returns A reference to the span representing the range of the buffer
   * specified by the \ref SubtreeBufferView.
   */
  const std::span<const std::byte>& getBufferAccessor() const {
    CESIUM_ASSERT(bufferAccessor.has_value());
    return *bufferAccessor;
  }

  /**
   * @brief Obtains the byte at the given index from the buffer used by the \ref
   * AvailabilityView.
   *
   * @warning Unsafe to use if isBufferView is false.
   * @returns The byte at the given index of the buffer accessor.
   */
  const std::byte& operator[](size_t i) const {
    CESIUM_ASSERT(bufferAccessor.has_value());
    return bufferAccessor.value()[i];
  }

  /**
   * @brief Obtains the size of the buffer used by the \ref AvailabilityView.
   *
   * @warning Unsafe to use if isBufferView is false.
   * @returns The \ref SubtreeBufferView::byteLength "byteLength" property of
   * the \ref SubtreeBufferView.
   */
  size_t size() const { return pBufferView->byteLength; }

private:
  const SubtreeBufferView* pBufferView;
  const ConstantAvailability* pConstant;
  std::optional<std::span<const std::byte>> bufferAccessor;
};
} // namespace CesiumGeometry
