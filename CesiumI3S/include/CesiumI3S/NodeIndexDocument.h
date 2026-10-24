#pragma once
#include <CesiumI3S/FeatureData.h>
#include <CesiumI3S/Library.h>
#include <CesiumI3S/LodSelection.h>
#include <CesiumI3S/Node.h>
#include <CesiumI3S/Resource.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Legacy 3dNodeIndexDocument payload (nodeIndexDocument.cmn.md). Used
 * when the store exposes individual node JSON documents rather than node pages.
 */
struct CESIUMI3S_API NodeIndexDocument {
  /** @brief Tree Key ID of this node. */
  std::string id;
  /** @brief Depth of this node in the tree hierarchy. */
  std::optional<uint32_t> level;
  /** @brief Version of this node (store update session ID). */
  std::optional<std::string> version;
  /** @brief Minimum bounding sphere. */
  std::optional<MinimumBoundingSphere> mbs;
  /** @brief Oriented bounding box. */
  std::optional<OrientedBoundingBox> obb;
  /** @brief Local-to-world transformation matrix. */
  std::optional<std::array<double, 16>> transform;
  /** @brief Reference to the parent node. */
  std::optional<NodeReference> parentNode;
  /** @brief References to child nodes. */
  std::optional<std::vector<NodeReference>> children;
  /** @brief References to sibling nodes. */
  std::optional<std::vector<NodeReference>> neighbors;
  /** @brief Shared resource reference (deprecated, I3S 1.6). */
  std::optional<Resource> sharedResource;
  /** @brief Feature data resource references. */
  std::optional<std::vector<Resource>> featureData;
  /** @brief Geometry resource references. */
  std::optional<std::vector<Resource>> geometryData;
  /** @brief Texture resource references. */
  std::optional<std::vector<Resource>> textureData;
  /** @brief Attribute resource references. */
  std::optional<std::vector<Resource>> attributeData;
  /** @brief LoD selection metrics for this node. */
  std::vector<LodSelection> lodSelection;
  /** @brief Inline feature data (deprecated, I3S 1.6). */
  std::optional<std::vector<FeatureData>> features;
};

} // namespace CesiumI3S
