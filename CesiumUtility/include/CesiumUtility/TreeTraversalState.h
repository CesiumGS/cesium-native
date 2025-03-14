#pragma once

#include <CesiumUtility/Assert.h>

#include <unordered_map>
#include <vector>

namespace CesiumUtility {

/**
 * @brief Associates state (arbitrary data) with each node during partial,
 * depth-first traversal of a tree. Then, during a later traversal of a
 * potentially different subset of the same tree, the state previously
 * associated with each node can be looked up.
 *
 * In order to operate efficiently, this class makes some assumptions. Violation
 * of these assumptions can lead to undefined behavior.
 *
 * 1. Nodes are identified by pointer. It is _not_ required that all nodes
 * previously traversed remain valid in a later traversal. However, if a new
 * node instance is created at the same memory address as a previous one, it
 * will be considered the same node.
 * 2. The entire tree is not necessarily traversed each time. However, if any
 * children of a tile are traversed, then _all_ children of the tile must be
 * traversed.
 * 3. The order of traversal of children must be the same every time.
 * 4. A node that previously had no children may gain them. A node that
 * previously had children may lose all of them. However, partial updates of the
 * children of a node are not allowed.
 *
 * @tparam TNodePointer The type of each node in the tree.
 * @tparam TState The state to associate with each node.
 */
template <typename TNodePointer, typename TState> class TreeTraversalState {
public:
  void beginTraversal() {
    std::swap(this->_previousTraversal, this->_currentTraversal);
    this->_currentTraversal.clear();
    this->_previousTraversalIndex = -1;
    this->_currentTraversalIndex = -1;
    this->_previousTraversalNextNodeIndex = 0;
  }

  void beginNode(TNodePointer pNode) {
    this->_currentTraversalIndex = this->_currentTraversal.size();

    int64_t parentPreviousTraversalIndex = this->_previousTraversalIndex;
    this->_previousTraversalIndex = this->_previousTraversalNextNodeIndex;
    ++this->_previousTraversalNextNodeIndex;

    CESIUM_ASSERT(this->_previousTraversalIndex >= 0);

    this->_parentIndices.emplace_back(TraversalIndices{
        .previous = size_t(this->_previousTraversalIndex),
        .current = size_t(this->_currentTraversalIndex)});

    this->_currentTraversal.emplace_back(TraversalData{pNode, -1, TState()});

    // Find this node in the previous traversal, if it exists.
    if (size_t(this->_previousTraversalIndex) <
        this->_previousTraversal.size()) {
      TraversalData& previousData =
          this->_previousTraversal[this->_previousTraversalIndex];
      if (previousData.pNode != pNode) {
        // The node we're currently visiting does not match the one that was
        // visited here in the previous traversal. There are two
        // possibilities:
        //
        // 1. We're visiting the first child of the previous node, and in the
        // previous traversal the previous node either didn't have any
        // children or they weren't visited.
        // 2. We're visiting the next sibling of the previous node, because
        // the previous node either doesn't have any children or they're not
        // being visited. In the previous traversal, the previous node had
        // children and we visited them.
        //
        // We can distinguish these cases by looking at the previous entry's
        // `nextSiblingIndex`. In case (1), the `nextSiblingIndex` will equal
        // the current `previousTraversalIndex`. In case (2), it will tell us
        // how many nodes in the previous traversal to skip.
        if (parentPreviousTraversalIndex < 0) {
          // If we get here, it means the root tile changed since the last
          // traversal. None of the previous results are useful.
          --this->_previousTraversalNextNodeIndex;
        } else {
          TraversalData& parentData =
              this->_previousTraversal[parentPreviousTraversalIndex];
          if (parentData.nextSiblingIndex == parentPreviousTraversalIndex + 1) {
            // Case 1 - visiting a child that was previous unvisited.
            // Hold the position in the previous traversal.
            --this->_previousTraversalNextNodeIndex;
          } else {
            // Case 2 - skipping children that were previously visited.
            this->_previousTraversalIndex = parentData.nextSiblingIndex;
            this->_previousTraversalNextNodeIndex =
                this->_previousTraversalIndex + 1;
            CESIUM_ASSERT(
                this->_previousTraversalIndex >= 0 &&
                size_t(this->_previousTraversalIndex) <
                    this->_previousTraversal.size() &&
                this->_previousTraversal[this->_previousTraversalIndex].pNode ==
                    pNode);
          }
        }
      }
    }
  }

  const TState* previousState() {
    const TraversalData* pData = this->previousData();
    return pData ? &pData->state : nullptr;
  }

  TState& currentState() {
    TraversalData& data = this->currentData();
    return data.state;
  }

  void finishNode([[maybe_unused]] TNodePointer pNode) {
    CESIUM_ASSERT(!this->_currentTraversal.empty());
    CESIUM_ASSERT(!this->_parentIndices.empty());
    CESIUM_ASSERT(
        int64_t(this->_parentIndices.back().current) ==
        this->_currentTraversalIndex);
    CESIUM_ASSERT(
        int64_t(this->_parentIndices.back().previous) ==
        this->_previousTraversalIndex);
    CESIUM_ASSERT(
        this->_currentTraversalIndex >= 0 &&
        size_t(this->_currentTraversalIndex) < this->_currentTraversal.size());

    // Now that this node is done, skip its subtree, if any, in the previous
    // traversal. If this finished node doesn't exist in the previous traversal,
    // look for the next previous node at the current position.
    const TraversalData* pPreviousData = this->previousData();
    if (pPreviousData) {
      this->_previousTraversalNextNodeIndex = pPreviousData->nextSiblingIndex;
    }

    this->_parentIndices.pop_back();

    CESIUM_ASSERT(
        this->_currentTraversal[this->_currentTraversalIndex].pNode == pNode);
    this->_currentTraversal[this->_currentTraversalIndex].nextSiblingIndex =
        int64_t(this->_currentTraversal.size());

    this->_currentTraversalIndex = -1;
    this->_previousTraversalIndex = -1;

    if (!this->_parentIndices.empty()) {
      const TraversalIndices& indices = this->_parentIndices.back();
      this->_currentTraversalIndex = indices.current;
      this->_previousTraversalIndex = indices.previous;
    }
  }

  template <typename Func> void forEachPreviousChild(Func&& callback) const {
    const TraversalData* pPrevious = this->previousData();
    if (!pPrevious)
      return;

    for (size_t i = size_t(this->_previousTraversalIndex + 1);
         i < size_t(pPrevious->nextSiblingIndex);) {
      const TraversalData& data = this->_previousTraversal[i];
      callback(data.pNode, data.state);
      CESIUM_ASSERT(
          data.nextSiblingIndex >= 0 && size_t(data.nextSiblingIndex) > i);
      i = size_t(data.nextSiblingIndex);
    }
  }

  template <typename Func>
  void forEachPreviousDescendant(Func&& callback) const {
    const TraversalData* pPrevious = this->previousData();
    if (!pPrevious)
      return;

    for (size_t i = size_t(this->_previousTraversalIndex + 1);
         i < size_t(pPrevious->nextSiblingIndex);
         ++i) {
      const TraversalData& data = this->_previousTraversal[i];
      callback(data.pNode, data.state);
    }
  }

  template <typename Func> void forEachCurrentDescendant(Func&& callback) {
    for (size_t i = size_t(this->_currentTraversalIndex + 1);
         i < this->_currentTraversal.size();
         ++i) {
      TraversalData& data = this->_currentTraversal[i];
      callback(data.pNode, data.state);
    }
  }

  std::unordered_map<TNodePointer, TState> slowlyGetCurrentStates() const {
    return this->slowlyGetStates(this->_currentTraversal);
  }

  std::unordered_map<TNodePointer, TState> slowlyGetPreviousStates() const {
    return this->slowlyGetStates(this->_previousTraversal);
  }

private:
  struct TraversalData {
    TNodePointer pNode;
    int64_t nextSiblingIndex;
    TState state;
  };

  struct TraversalIndices {
    size_t previous;
    size_t current;
  };

  const TraversalData* previousData() const {
    if (this->_previousTraversalIndex >= 0 &&
        size_t(this->_previousTraversalIndex) <
            this->_previousTraversal.size()) {
      CESIUM_ASSERT(
          this->_currentTraversalIndex >= 0 &&
          size_t(this->_currentTraversalIndex) <
              this->_currentTraversal.size());
      TNodePointer pCurrentNode =
          this->_currentTraversal[this->_currentTraversalIndex].pNode;
      const TraversalData& previousData =
          this->_previousTraversal[this->_previousTraversalIndex];
      if (previousData.pNode == pCurrentNode) {
        return &previousData;
      }
    }

    return nullptr;
  }

  TraversalData& currentData() {
    CESIUM_ASSERT(
        this->_currentTraversalIndex >= 0 &&
        size_t(this->_currentTraversalIndex) < this->_currentTraversal.size());
    return this->_currentTraversal[this->_currentTraversalIndex];
  }

  std::unordered_map<TNodePointer, TState>
  slowlyGetStates(const std::vector<TraversalData>& traversalData) const {
    std::unordered_map<TNodePointer, TState> result;

    for (const TraversalData& data : traversalData) {
      result[data.pNode] = data.state;
    }

    return result;
  }

  std::vector<TraversalData> _previousTraversal;
  std::vector<TraversalData> _currentTraversal;
  std::vector<TraversalIndices> _parentIndices;
  int64_t _previousTraversalIndex = -1;
  int64_t _previousTraversalNextNodeIndex = 0;
  int64_t _currentTraversalIndex = -1;
};

} // namespace CesiumUtility
