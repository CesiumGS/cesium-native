#pragma once

#include <CesiumUtility/Assert.h>

#include <cstddef>
#include <iterator>
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

  class Differences;

  /**
   * @brief Compares the current traversal against the previous one. Provides an
   * iterator over all of the nodes that had a different state in the two
   * traversals.
   *
   * The iteration also includes:
   *
   *   * Each node that was visited previously but was not visited in the
   * current traversal.
   *   * Each node that was not visited previously but was visited in the
   * current traversal.
   *
   * This method should only be called after the {@link finishNode} for the
   * root node, and before {@link beginTraversal}. In other words, it should
   * not be called while a traversal is in progress.
   *
   * Starting a traversal after calling this method invalidates the returned
   * instance.
   */
  Differences differences() const noexcept {
    // Assert that a traversal is not currently in progress.
    CESIUM_ASSERT(this->_parentIndices.empty());

    return Differences{
        *this,
        this->_previousTraversal.size(),
        this->_currentTraversal.size()};
  }

#pragma region Differences Implementation

  /**
   * @brief Represents a single difference reported by {@link differences}.
   */
  struct Difference {
    /**
     * @brief The node with a different state.
     */
    const TNodePointer& pNode;

    /**
     * @brief The state of the node in the previous traversal, or a
     * default-constructed instance if the node was not visited at all in the
     * previous traversal.
     */
    const TState& previousState;

    /**
     * @brief The state of the node in the current traversal, or a
     * default-constructed instance if the node was not visited at all in the
     * current traversal.
     */
    const TState& currentState;

    // These operators allow a `Difference` instance to be returned by value
    // from the difference_iterator's `operator->` method.

    /** @private */
    Difference* operator->() { return this; }

    /** @private */
    const Difference* operator->() const { return this; }
  };

  /**
   * @brief The type of the iterator created by {@link Differences}.
   */
  class difference_iterator {
  public:
    /**
     * @brief The iterator category tag denoting this is a forward iterator.
     */
    using iterator_category = std::forward_iterator_tag;
    /**
     * @brief The type of value that is being iterated over.
     */
    using value_type = Difference;
    /**
     * @brief The type used to identify distance between iterators.
     *
     * This is `void` because there is no meaningful measure of distance between
     * tiles.
     */
    using difference_type = void;
    /**
     * @brief A pointer to the type being iterated over.
     */
    using pointer = const value_type*;
    /**
     * @brief A reference to the type being iterated over.
     */
    using reference = const value_type&;

    /**
     * @brief Returns a reference to the current difference being iterated.
     */
    value_type operator*() const noexcept;
    /**
     * @brief Returns a pointer to the current difference being iterated.
     */
    value_type operator->() const noexcept;

    /**
     * @brief Advances the iterator to the next difference (pre-incrementing).
     */
    difference_iterator& operator++() noexcept;
    /**
     * @brief Advances the iterator to the next difference (post-incrementing).
     */
    difference_iterator operator++(int) noexcept;

    /** @brief Checks if two iterators are at the same difference. */
    bool operator==(const difference_iterator& rhs) const noexcept;
    /** @brief Checks if two iterators are not at the same difference. */
    bool operator!=(const difference_iterator& rhs) const noexcept;

  private:
    explicit difference_iterator(
        const TreeTraversalState<TNodePointer, TState>* pState,
        int64_t previousIndex,
        int64_t currentIndex) noexcept;
    void advanceToNextDifference() noexcept;

    const TreeTraversalState<TNodePointer, TState>* _pState;
    // The index of the current difference in _previousTraversal.
    int64_t _previousIndex;
    // The index of the current difference in _currentTraversal.
    int64_t _currentIndex;
    // The next sibling index if we're currently skipping siblings. -1 if we're
    // not currently skipping siblings.
    int64_t _nextSiblingIndex;
    // If we're skipping siblings, this value is true when the skipped siblings
    // are from the previous traversal, or false if the skipped siblings are
    // from the current traversal.
    bool _skippingPrevious;

    static const inline TState DEFAULT_STATE{};

    friend class TreeTraversalState<TNodePointer, TState>;
  };

  /**
   * @brief Returned by the {@link differences} method to allow iteration over
   * the differences between two traversals of the same tree.
   */
  class Differences {
  public:
    /**
     * @brief Gets an iterator pointing to the first difference.
     */
    difference_iterator begin() const noexcept {
      return difference_iterator(this->_pState, 0, 0);
    }

    /**
     * @brief Gets an iterator pointing to one past the last difference.
     */
    difference_iterator end() const noexcept {
      return difference_iterator(
          this->_pState,
          int64_t(this->_previousTraversalSize),
          int64_t(this->_currentTraversalSize));
    }

  private:
    Differences(
        const TreeTraversalState& traversalState,
        size_t previousTraversalSize,
        size_t currentTraversalSize) noexcept
        : _pState(&traversalState),
          _previousTraversalSize(previousTraversalSize),
          _currentTraversalSize(currentTraversalSize) {}

    const TreeTraversalState* _pState;
    size_t _previousTraversalSize;
    size_t _currentTraversalSize;

    friend class TreeTraversalState;
  };

#pragma endregion

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

  template <typename TNodePointer, typename TState>
  friend class TreeTraversalStateDiffIterator;
};

#pragma region Differences Implementation

template <typename TNodePointer, typename TState>
TreeTraversalState<TNodePointer, TState>::Difference
TreeTraversalState<TNodePointer, TState>::difference_iterator::operator*()
    const noexcept {
  if (this->_nextSiblingIndex >= 0) {
    if (this->_skippingPrevious) {
      CESIUM_ASSERT(
          this->_previousIndex >= 0 &&
          size_t(this->_previousIndex) <
              this->_pState->_previousTraversal.size());

      const TreeTraversalState<TNodePointer, TState>::TraversalData& data =
          this->_pState->_previousTraversal[size_t(this->_previousIndex)];
      return value_type{
          .pNode = data.pNode,
          .previousState = data.state,
          .currentState = DEFAULT_STATE};
    } else {
      CESIUM_ASSERT(
          this->_currentIndex >= 0 &&
          size_t(this->_currentIndex) <
              this->_pState->_currentTraversal.size());

      const TreeTraversalState<TNodePointer, TState>::TraversalData& data =
          this->_pState->_currentTraversal[size_t(this->_currentIndex)];
      return value_type{
          .pNode = data.pNode,
          .previousState = DEFAULT_STATE,
          .currentState = data.state};
    }
  } else {
    CESIUM_ASSERT(
        this->_previousIndex >= 0 &&
        size_t(this->_previousIndex) <
            this->_pState->_previousTraversal.size());
    CESIUM_ASSERT(
        this->_currentIndex >= 0 &&
        size_t(this->_currentIndex) < this->_pState->_currentTraversal.size());

    const TreeTraversalState<TNodePointer, TState>::TraversalData&
        previousData =
            this->_pState->_previousTraversal[size_t(this->_previousIndex)];
    const TreeTraversalState<TNodePointer, TState>::TraversalData& currentData =
        this->_pState->_currentTraversal[size_t(this->_currentIndex)];

    CESIUM_ASSERT(previousData.pNode == currentData.pNode);

    return value_type{
        .pNode = previousData.pNode,
        .previousState = previousData.state,
        .currentState = currentData.state};
  }
}

template <typename TNodePointer, typename TState>
TreeTraversalState<TNodePointer, TState>::Difference
TreeTraversalState<TNodePointer, TState>::difference_iterator::operator->()
    const noexcept {
  return this->operator*();
}

template <typename TNodePointer, typename TState>
TreeTraversalState<TNodePointer, TState>::difference_iterator&
TreeTraversalState<TNodePointer, TState>::difference_iterator::
operator++() noexcept {
  this->advanceToNextDifference();
  return *this;
}

template <typename TNodePointer, typename TState>
TreeTraversalState<TNodePointer, TState>::difference_iterator
TreeTraversalState<TNodePointer, TState>::difference_iterator::operator++(
    int) noexcept {
  TreeTraversalStateDiffIterator<TNodePointer, TState> result = *this;
  return ++result;
}

template <typename TNodePointer, typename TState>
bool TreeTraversalState<TNodePointer, TState>::difference_iterator::operator==(
    const TreeTraversalState::difference_iterator& rhs) const noexcept {
  return this->_previousIndex == rhs._previousIndex &&
         this->_currentIndex == rhs._currentIndex &&
         this->_pState == rhs._pState;
}

template <typename TNodePointer, typename TState>
bool TreeTraversalState<TNodePointer, TState>::difference_iterator::operator!=(
    const TreeTraversalState::difference_iterator& rhs) const noexcept {
  return !(*this == rhs);
}

template <typename TNodePointer, typename TState>
TreeTraversalState<TNodePointer, TState>::difference_iterator::
    difference_iterator(
        const TreeTraversalState<TNodePointer, TState>* pState,
        int64_t previousIndex,
        int64_t currentIndex) noexcept
    : _pState(pState),
      _previousIndex(previousIndex),
      _currentIndex(currentIndex),
      _nextSiblingIndex(-1),
      _skippingPrevious(false) {
  bool validPreviousIndex =
      previousIndex < int64_t(pState->_previousTraversal.size());
  bool validCurrentIndex =
      currentIndex < int64_t(pState->_currentTraversal.size());
  if (validPreviousIndex && validCurrentIndex) {
    const TreeTraversalState<TNodePointer, TState>::TraversalData&
        previousData = pState->_previousTraversal[size_t(previousIndex)];
    const TreeTraversalState<TNodePointer, TState>::TraversalData& currentData =
        pState->_currentTraversal[size_t(currentIndex)];
    CESIUM_ASSERT(previousData.pNode == currentData.pNode);
    if (previousData.state != currentData.state) {
      // The root node has a different state in the two traversals, so this is
      // our first difference.
      return;
    } else {
      // The root node has the same state in both traversals. Advanced to the
      // next difference.
      this->advanceToNextDifference();
    }
  } else if (validPreviousIndex) {
    // There are no nodes at all in the current traversal, so all previous
    // states are differences.
    this->_skippingPrevious = true;
    this->_nextSiblingIndex = int64_t(pState->_previousTraversal.size());
  } else if (validCurrentIndex) {
    // There were no nodes at all in the previous traversal, so all current
    // states are differences.
    this->_skippingPrevious = false;
    this->_nextSiblingIndex = int64_t(pState->_currentTraversal.size());
  }

  // If none of the conditions above are met, then either both traversals are
  // empty and there are no differences, or this is the `end` iterator.
}

template <typename TNodePointer, typename TState>
void TreeTraversalState<TNodePointer, TState>::difference_iterator::
    advanceToNextDifference() noexcept {
  bool first = true;

  if (this->_nextSiblingIndex >= 0) {
    // Currently enumerating descendants that were not visited at all in the
    // other traversal.
    if (this->_skippingPrevious) {
      ++this->_previousIndex;
      if (this->_previousIndex < this->_nextSiblingIndex) {
        return;
      }
    } else {
      ++this->_currentIndex;
      if (this->_currentIndex < this->_nextSiblingIndex) {
        return;
      }
    }

    // If we don't return above, then the unmatched descendant enumeration is
    // now complete.
    this->_nextSiblingIndex = -1;
    first = false;
  }

  while (this->_previousIndex <
             int64_t(this->_pState->_previousTraversal.size()) &&
         this->_currentIndex <
             int64_t(this->_pState->_currentTraversal.size())) {
    const TreeTraversalState<TNodePointer, TState>::TraversalData&
        previousData =
            this->_pState->_previousTraversal[size_t(this->_previousIndex)];
    const TreeTraversalState<TNodePointer, TState>::TraversalData& currentData =
        this->_pState->_currentTraversal[size_t(this->_currentIndex)];

    CESIUM_ASSERT(previousData.pNode == currentData.pNode);
    if (previousData.pNode != currentData.pNode) {
      // This shouldn't happen. Stop the iteration by setting this iterator
      // equal to the end() iterator.
      this->_previousIndex = int64_t(this->_pState->_previousTraversal.size());
      this->_currentIndex = int64_t(this->_pState->_currentTraversal.size());
      return;
    }

    if (!first && previousData.state != currentData.state) {
      // Current node has a different state in the two traversals.
      return;
    }

    first = false;

    bool previousTraversalVisitedChildren =
        previousData.nextSiblingIndex > this->_previousIndex + 1;
    bool currentTraversalVisitedChildren =
        currentData.nextSiblingIndex > this->_currentIndex + 1;

    ++this->_previousIndex;
    ++this->_currentIndex;

    if (previousTraversalVisitedChildren && !currentTraversalVisitedChildren) {
      // No descendants in current traversal, so every previous traversal
      // descendant is a difference.
      this->_skippingPrevious = true;
      this->_nextSiblingIndex = previousData.nextSiblingIndex;
      return;
    } else if (
        currentTraversalVisitedChildren && !previousTraversalVisitedChildren) {
      // No descendants in previous traversal, so every current traversal
      // descendant is a difference.
      this->_skippingPrevious = false;
      this->_nextSiblingIndex = currentData.nextSiblingIndex;
      return;
    }
  }

  // If we get here without returning, we're done iterating. We should be at the
  // end of both traversals.
  CESIUM_ASSERT(
      this->_previousIndex ==
      int64_t(this->_pState->_previousTraversal.size()));
  CESIUM_ASSERT(
      this->_currentIndex == int64_t(this->_pState->_currentTraversal.size()));
}

#pragma endregion

} // namespace CesiumUtility
