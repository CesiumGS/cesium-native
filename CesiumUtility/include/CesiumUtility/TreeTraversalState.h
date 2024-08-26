#pragma once

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
 * @tparam TNode The type of each node in the tree.
 * @tparam TState The state to associate with each node.
 */
template <typename TNode, typename TState> class TreeTraversalState {
public:
};

} // namespace CesiumUtility
