#pragma once

namespace Cesium3DTiles {
/**
 * @brief Indicates how an implicit tile is subdivided.
 */
enum class ImplicitTileSubdivisionScheme {
  /**
   * @brief Implicit tiles are divided into four children, forming a quadree.
   */
  Quadtree,

  /**
   * @brief Implicit tiles are divided into eight children, forming an octree.
   */
  Octree
};
} // namespace Cesium3DTiles
