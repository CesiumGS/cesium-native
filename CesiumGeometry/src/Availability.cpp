
#include <CesiumGeometry/Availability.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace CesiumGeometry {

namespace AvailabilityUtilities {
static const uint8_t ones_in_byte[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

uint8_t countOnesInByte(uint8_t _byte) { return ones_in_byte[_byte]; }

uint32_t countOnesInBuffer(std::span<const std::byte> buffer) {
  uint32_t count = 0;
  for (const std::byte& byte : buffer) {
    count += countOnesInByte((uint8_t)byte);
  }
  return count;
}
} // namespace AvailabilityUtilities

AvailabilityNode::AvailabilityNode() noexcept
    : subtree(std::nullopt), childNodes() {}

void AvailabilityNode::setLoadedSubtree(
    AvailabilitySubtree&& subtree_,
    uint32_t maxChildrenSubtrees) noexcept {
  this->subtree = std::make_optional<AvailabilitySubtree>(std::move(subtree_));

  AvailabilityAccessor subtreeAvailabilityAccessor(
      this->subtree->subtreeAvailability,
      *this->subtree);

  size_t childNodesCount = 0;
  if (subtreeAvailabilityAccessor.isConstant()) {
    if (subtreeAvailabilityAccessor.getConstant()) {
      childNodesCount = maxChildrenSubtrees;
    }
  } else if (subtreeAvailabilityAccessor.isBufferView()) {
    childNodesCount = AvailabilityUtilities::countOnesInBuffer(
        subtreeAvailabilityAccessor.getBufferAccessor());
  } else {
    return;
  }

  this->childNodes.resize(childNodesCount);
}

AvailabilityAccessor::AvailabilityAccessor(
    const AvailabilityView& view,
    const AvailabilitySubtree& subtree) noexcept
    : pBufferView(std::get_if<SubtreeBufferView>(&view)),
      pConstant(std::get_if<ConstantAvailability>(&view)) {
  if (this->pBufferView) {
    if (this->pBufferView->buffer < subtree.buffers.size()) {
      const std::vector<std::byte>& buffer =
          subtree.buffers[this->pBufferView->buffer];
      if (this->pBufferView->byteOffset + this->pBufferView->byteLength <=
          buffer.size()) {
        this->bufferAccessor = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(buffer.data()) +
                this->pBufferView->byteOffset,
            this->pBufferView->byteLength);
      }
    }
  }
}

} // namespace CesiumGeometry
