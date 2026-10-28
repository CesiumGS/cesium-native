#pragma once
#include <CesiumI3S/Library.h>
#include <CesiumI3S/LodSelection.h>
#include <CesiumI3S/Mesh.h>
#include <CesiumI3S/SpatialReference.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Defines the paging structure of the node index
 * (nodePageDefinition.cmn.md). */
struct CESIUMI3S_API NodePageDefinition {
  /** @brief Number of nodes per page. Must be a power-of-two less than 4096. */
  uint32_t nodesPerPage = 64;
  /** @brief Index of the root node. Default=0. */
  uint32_t rootIndex = 0;
  /** @brief Defines the meaning of nodes[].lodThreshold for this layer. */
  LodSelection::Metric lodSelectionMetricType =
      LodSelection::Metric::MaxScreenThreshold;
};

/** @brief Pointer to a referenced node with metadata (nodeReference.cmn.md). */
struct CESIUMI3S_API NodeReference {
  /** @brief Tree Key ID of the referenced node. */
  std::string id;
  /** @brief Minimum bounding sphere. */
  std::optional<MinimumBoundingSphere> mbs;
  /** @brief Relative URL to the referenced node. */
  std::optional<std::string> href;
  /** @brief Version (store update session ID) of the referenced node. */
  std::optional<std::string> version;
  /** @brief Number of features in the referenced node and its descendants. */
  std::optional<uint64_t> featureCount;
  /** @brief Oriented bounding box of the referenced node. */
  std::optional<OrientedBoundingBox> obb;
};

/** @brief A single node in the node page flat array (node.cmn.md). */
struct CESIUMI3S_API Node {
  /** @brief The index in the node array. */
  uint32_t index = 0;
  /** @brief The index of the parent node in the node array. */
  std::optional<uint32_t> parentIndex;
  /** @brief LoD switching threshold. See
   * nodePageDefinition.lodSelectionMetricType. */
  std::optional<double> lodThreshold;
  /** @brief Oriented bounding box for this node. */
  OrientedBoundingBox obb;
  /** @brief Indices of the children nodes. */
  std::optional<std::vector<uint32_t>> children;
  /** @brief The mesh for this node. Only a single mesh is supported. */
  std::optional<Mesh> mesh;
};

/** @brief A page of nodes from the node index (nodePage.cmn.md). */
struct CESIUMI3S_API NodePage {
  /** @brief Array of nodes. */
  std::vector<Node> nodes;
};

} // namespace CesiumI3S
