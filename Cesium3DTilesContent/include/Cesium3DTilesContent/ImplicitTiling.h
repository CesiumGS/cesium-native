#pragma once

#include <string>

namespace CesiumGeometry {
struct QuadtreeTileID;
struct OctreeTileID;
} // namespace CesiumGeometry

namespace Cesium3DTilesContent {

/**
 * @brief Helper functions for working with 3D Tiles implicit tiling.
 */
class ImplicitTiling {
public:
  /**
   * @brief Resolves a templatized implicit tiling URL with a quadtree tile ID.
   *
   * @param baseUrl The base URL that is used to resolve the urlTemplate if it
   * is a relative path.
   * @param urlTemplate The templatized URL.
   * @param quadtreeID The quadtree ID to use in resolving the parameters in the
   * URL template.
   * @return The resolved URL.
   */
  static std::string resolveUrl(
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const CesiumGeometry::QuadtreeTileID& quadtreeID);

  /**
   * @brief Resolves a templatized implicit tiling URL with an octree tile ID.
   *
   * @param baseUrl The base URL that is used to resolve the urlTemplate if it
   * is a relative path.
   * @param urlTemplate The templatized URL.
   * @param octreeID The octree ID to use in resolving the parameters in the
   * URL template.
   * @return The resolved URL.
   */
  static std::string resolveUrl(
      const std::string& baseUrl,
      const std::string& urlTemplate,
      const CesiumGeometry::OctreeTileID& octreeID);
};

} // namespace Cesium3DTilesContent
