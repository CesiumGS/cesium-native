#pragma once

#include <CesiumUtility/Assert.h>

#include <cstddef>
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
 * children of a node are traversed, then _all_ children of the node must be
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
  /**
   * @brief Gets the total number of nodes that were visited in the previous
   * traversal.
   */
  size_t getNodeCountInPreviousTraversal() const {
    return this->_previousTraversal.size();
  }

  /**
   * @brief Gets the total number of nodes that have been visited so far in the
   * current traversal.
   */
  size_t getNodeCountInCurrentTraversal() const {
    return this->_currentTraversal.size();
  }

  /**
   * @brief Begins a new traversal of the tree. The "current" and "previous"
   * traversals are swapped, and then the new "current" traversal is cleared.
   */
  void beginTraversal() {
    // If this assertion fails, it indicates a traversal is already in progress.
    CESIUM_ASSERT(this->_parentIndices.empty());

    this->_previousTraversal.swap(this->_currentTraversal);
    this->_currentTraversal.clear();
    this->_previousTraversalNextNodeIndex = 0;
  }

  /**
   * @brief Begins traversing a node in the tree. This node becomes the
   * "current" node.
   *
   * When `beginNode` is called for node A, and then for node B, without an
   * intervening call to `finishNode`, that indicates that B is a child of A.
   *
   * @param pNode The node traversed.
   */
  void beginNode(const TNodePointer& pNode) {
    int64_t currentTraversalIndex = int64_t(this->_currentTraversal.size());
    int64_t previousTraversalIndex = this->_previousTraversalNextNodeIndex;

    if (previousTraversalIndex >= 0 &&
        size_t(previousTraversalIndex) < this->_previousTraversal.size()) {
      const TraversalData& previousData =
          this->_previousTraversal[size_t(previousTraversalIndex)];
      if (previousData.pNode == pNode) {
        ++this->_previousTraversalNextNodeIndex;
      } else {
        previousTraversalIndex = -1;
      }
    } else {
      previousTraversalIndex = -1;
    }

    this->_parentIndices.emplace_back(TraversalIndices{
        .previous = previousTraversalIndex,
        .current = currentTraversalIndex});

    this->_currentTraversal.emplace_back(TraversalData{pNode, -1, TState()});
  }

  /**
   * @brief Gets the current node in the traversal.
   *
   * When {@link beginNode} is called, the node passed to it becomes the
   * current one. When {@link finishNode} is called, the parent node of the
   * one passed to it becomes the current one.
   *
   * @return The current node, or nullptr if no traversal is in progress.
   */
  TNodePointer getCurrentNode() const noexcept {
    if (this->_parentIndices.empty())
      return nullptr;

    return this->currentData().pNode;
  }

  /**
   * @brief Determines if the current node was visited in the previous
   * traversal.
   *
   * @return true if the current node was visited in the previous traversal;
   * otherwise, false.
   */
  bool wasCurrentNodePreviouslyTraversed() const noexcept {
    return this->previousState() != nullptr;
  }

  /**
   * @brief Gets the state of the current node on the previous traversal. The
   * current node is the one for which {@link beginNode} was most recently
   * called.
   *
   * @return The state on the previous traversal, or `nullptr` if the current
   * node was not traversed during the previous traversal.
   */
  const TState* previousState() const {
    const TraversalData* pData = this->previousData();
    return pData ? &pData->state : nullptr;
  }

  /**
   * @brief Gets the state of the current node during the current traversal. The
   * current node is the one for which {@link beginNode} was most recently
   * called.
   *
   * @return The state object for this node on the current traversal. It may be
   * both read and written.
   */
  TState& currentState() {
    TraversalData& data = this->currentData();
    return data.state;
  }

  /**
   * @brief Gets the state of the current node during the current traversal. The
   * current node is the one for which {@link beginNode} was most recently
   * called.
   *
   * @return The state object for this node on the current traversal.
   */
  const TState& currentState() const {
    const TraversalData& data = this->currentData();
    return data.state;
  }

  /**
   * @brief Ends traversal of the given node.
   *
   * This method must be called in the opposite order of calls to
   * {@link finishNode}.
   *
   * After `finishNode`, this node's parent node becomes the "current" node.
   * {@link previousState} and {@link currentState} will return
   * the states of this node's parent. A call to {@link beginNode} will
   * delineate a new child of that same parent.
   *
   * @param pNode The node that is done being traversed.
   */
  void finishNode([[maybe_unused]] const TNodePointer& pNode) {
    // An assertion failure here indicates mismatched calls to beginNode /
    // finishNode.
    CESIUM_ASSERT(!this->_currentTraversal.empty());
    CESIUM_ASSERT(!this->_parentIndices.empty());
    CESIUM_ASSERT(this->currentData().pNode == pNode);

    this->currentData().nextSiblingIndex =
        int64_t(this->_currentTraversal.size());

    // Now that this node is done, skip its subtree, if any, in the previous
    // traversal. If this finished node doesn't exist in the previous traversal,
    // look for the next node in the previous traversal at the current position.
    const TraversalData* pPreviousData = this->previousData();
    if (pPreviousData) {
      CESIUM_ASSERT(pPreviousData->nextSiblingIndex >= 0);
      this->_previousTraversalNextNodeIndex = pPreviousData->nextSiblingIndex;
    }

    this->_parentIndices.pop_back();
  }

  /**
   * @brief Invokes a callback for each child of the current node that was
   * traversed in the previous traversal.
   *
   * If the current node or its children were not traversed in the previous
   * traversal, this method returns without invoking the callback at all.
   *
   * @tparam Func The type of the function to invoke.
   * @param callback The function to invoke for each previously-traversed child.
   * The function is passed the `TNodePointer` and the `TState`.
   */
  template <typename Func> void forEachPreviousChild(Func&& callback) const {
    int64_t parentPreviousIndex = this->previousDataIndex();
    if (parentPreviousIndex < 0) {
      // Current node was not previously traversed.
      return;
    }

    const TraversalData& parentPreviousData =
        this->_previousTraversal[size_t(parentPreviousIndex)];

    for (size_t i = size_t(parentPreviousIndex + 1);
         i < size_t(parentPreviousData.nextSiblingIndex);) {
      CESIUM_ASSERT(i < this->_previousTraversal.size());
      const TraversalData& data = this->_previousTraversal[i];
      callback(data.pNode, data.state);
      CESIUM_ASSERT(
          data.nextSiblingIndex >= 0 && size_t(data.nextSiblingIndex) > i);
      i = size_t(data.nextSiblingIndex);
    }
  }

  /**
   * @brief Invokes a callback for each descendant (children, grandchildren,
   * etc.) of the current node that was traversed in the previous traversal.
   *
   * If the current node or its children were not traversed in the previous
   * traversal, this method returns without invoking the callback at all.
   *
   * @tparam Func The type of the function to invoke.
   * @param callback The function to invoke for each previously-traversed
   * descendant. The function is passed the `TNodePointer` and the `TState`.
   */
  template <typename Func>
  void forEachPreviousDescendant(Func&& callback) const {
    int64_t parentPreviousIndex = this->previousDataIndex();
    if (parentPreviousIndex < 0) {
      // Current node was not previously traversed.
      return;
    }

    const TraversalData& parentPreviousData =
        this->_previousTraversal[size_t(parentPreviousIndex)];

    for (size_t i = size_t(parentPreviousIndex + 1);
         i < size_t(parentPreviousData.nextSiblingIndex);
         ++i) {
      CESIUM_ASSERT(i < this->_previousTraversal.size());
      const TraversalData& data = this->_previousTraversal[i];
      callback(data.pNode, data.state);
    }
  }

  /**
   * @brief Invokes a callback for each descendant (children, grandchildren,
   * etc.) of the current node that has been traversed so far in the current
   * traversal.
   *
   * If the current node's children were not traversed in the current
   * traversal, this method returns without invoking the callback at all.
   *
   * @tparam Func The type of the function to invoke.
   * @param callback The function to invoke for each descendant of the current
   * tile that has been traversed in the current traversal. The function is
   * passed the `TNodePointer` and the `TState`.
   */
  template <typename Func> void forEachCurrentDescendant(Func&& callback) {
    int64_t parentCurrentIndex = this->currentDataIndex();
    CESIUM_ASSERT(parentCurrentIndex >= 0);

    const TraversalData& parentCurrentData =
        this->_currentTraversal[size_t(parentCurrentIndex)];

    size_t endIndex = parentCurrentData.nextSiblingIndex >= 0
                          ? size_t(parentCurrentData.nextSiblingIndex)
                          : this->_currentTraversal.size();

    for (size_t i = size_t(parentCurrentIndex + 1); i < endIndex; ++i) {
      TraversalData& data = this->_currentTraversal[i];
      callback(data.pNode, data.state);
    }
  }

  /**
   * @brief Gets a mapping of nodes to states for the current traversal.
   *
   * This is an inherently slow operation that should only be used in debug and
   * test code.
   *
   * @return The mapping of nodes to current traversal states.
   */
  std::unordered_map<TNodePointer, TState> slowlyGetCurrentStates() const {
    return this->slowlyGetStates(this->_currentTraversal);
  }

  /**
   * @brief Gets a mapping of nodes to states for the previous traversal.
   *
   * This is an inherently slow operation that should only be used in debug and
   * test code.
   *
   * @return The mapping of nodes to previous traversal states.
   */
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
    int64_t previous;
    int64_t current;
  };

  int64_t previousDataIndex() const {
    CESIUM_ASSERT(!this->_parentIndices.empty());

    int64_t result = this->_parentIndices.back().previous;

    CESIUM_ASSERT(
        result == -1 ||
        (result >= 0 && size_t(result) < this->_previousTraversal.size()));

    return result;
  }

  int64_t currentDataIndex() const {
    CESIUM_ASSERT(!this->_parentIndices.empty());

    // An assertion failure here may indicate beginTraversal wasn't called.
    CESIUM_ASSERT(
        this->_parentIndices.back().current >= 0 &&
        this->_parentIndices.back().current <
            static_cast<int64_t>(this->_currentTraversal.size()));

    return this->_parentIndices.back().current;
  }

  const TraversalData* previousData() const {
    int64_t previousIndex = this->previousDataIndex();
    if (previousIndex < 0)
      return nullptr;

    const TraversalData& previousData =
        this->_previousTraversal[size_t(previousIndex)];

    CESIUM_ASSERT(previousData.pNode == this->currentData().pNode);

    return &previousData;
  }

  TraversalData& currentData() {
    int64_t currentIndex = this->currentDataIndex();
    CESIUM_ASSERT(currentIndex >= 0);
    return this->_currentTraversal[size_t(currentIndex)];
  }

  const TraversalData& currentData() const {
    int64_t currentIndex = this->currentDataIndex();
    CESIUM_ASSERT(currentIndex >= 0);
    return this->_currentTraversal[size_t(currentIndex)];
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

  // A stack of indices into the previous and current traversals. When
  // `beginNode` is called, the index of that node within each traversal is
  // pushed onto the end of this vector. This is always the index of the _last_
  // node in the `_currentTraversal`, because `beginNode` always adds a new
  // entry to the end of the `currentTraversal`. The previous traversal index
  // may be -1 if the current node was not visited at all in the previous
  // traversal. `finishNode` pops the last entry off the end of this array.
  std::vector<TraversalIndices> _parentIndices;

  // The index of the next node in the previous traversal. In `beginNode`, this
  // new node is added to the end of `_currentTraversal`. In
  // `_previousTraversal`, if it exists at all, it will be found at this index.
  int64_t _previousTraversalNextNodeIndex = 0;
};

} // namespace CesiumUtility
