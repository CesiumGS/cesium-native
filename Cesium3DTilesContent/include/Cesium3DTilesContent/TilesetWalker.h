#pragma once

#include <gsl/span>

namespace CesiumAsync {
class IAssetRequest;
}

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTiles {
struct Tile;
struct Tileset;
struct ImplicitTiling;
} // namespace Cesium3DTiles

namespace Cesium3DTilesContent {

/**
 * @brief Controls the traversal of a tileset with {@link TilesetWalker}.
 */
class TilesetWalkerControl {
public:
  /**
   * @brief Requests that the current tile's children, if any, be loaded and
   * visited.
   *
   * If the children are to be visited, the following happens after the current
   * `visitTile` returns:
   *   1. The information about the children is loaded asynchronously, if
   * necessary.
   *   2. {@link TilesetVisitor::visitChildrenBegin} is called.
   *   3. {@link TilesetVisitor::visitTile} is called for each child tile.
   *   4. {@link TilesetVisitor::visitChildrenEnd} is called.
   *
   * Note that depending on which `TilesetWalkerControl` methods are called from
   * within `visitTile`, further visits of descendant tiles, content, etc. may
   * happen between steps (3) and (4).
   *
   * @param visit True to visit children, false to skip visiting children.
   */
  TilesetWalkerControl& visitChildren(bool visit = true);

  /**
   * @brief Requests that the current tile's content be loaded and visited.
   *
   * If the content is to be visited, the following happens after the current
   * `visitTile` returns:
   *   1. The tile's content is loaded asynchronously.
   *   2. Exactly one of the following content visitation methods is invoked:
   *      {@link TilesetVisitor::visitNoContent},
   *      {@link TilesetVisitor::visitModelContent},
   *      {@link TilesetVisitor::visitExternalContent}, or
   *      {@link TilesetVisitor::visitUnknownContent}.
   *
   * @param visit True to visit content, false to skip visiting content.
   */
  TilesetWalkerControl& visitContent(bool visit = true);

  /**
   * @brief Requests that the current tile's implicit subdivision, if any, is
   * visited.
   *
   * If the current tile has an implicit subdivision and it is to be visited,
   * the following happens after the current `visitTile` returns:
   *   1. {@link TilesetVisitor::visitImplicitSubdivisionBegin} is called.
   *   2. If `visitChildren` is also enabled, {@link TilesetVisitor::visitTile}
   *      is called for the implicitly-defined children of this tile.
   *   3. {@link TilesetVisitor::visitImplicitSubdivisionEnd} is called.
   *
   * @param visit True to visit an implicit subdivision, false to skip it.
   */
  TilesetWalkerControl& visitImplicitSubdivision(bool visit = true);
};

class TilesetVisitor {
public:
  /**
   * @brief Visits an explicit or implicit tile.
   *
   * @param control An object allowing control of further traversal.
   * @param tile The tile to visit.
   */
  virtual void visitTile(TilesetWalkerControl& control, Tile& tile) = 0;

  virtual void
  visitChildrenBegin(TilesetWalkerControl& control, Tile& tile) = 0;
  virtual void visitChildrenEnd(TilesetWalkerControl& control, Tile& tile) = 0;

  virtual void visitImplicitSubdivisionBegin(
      TilesetWalkerControl& control,
      Tile& tile,
      Cesium3DTiles::ImplicitTiling& implicit) = 0;
  virtual void visitImplicitSubdivisionEnd(
      TilesetWalkerControl& control,
      Tile& tile,
      Cesium3DTiles::ImplicitTiling& implicit) = 0;

  virtual void visitNoContent(TilesetWalkerControl& control, Tile& tile) = 0;
  virtual void visitModelContent(
      TilesetWalkerControl& control,
      Tile& tile,
      CesiumAsync::IAssetRequest& request,
      CesiumGltf::Model& model) = 0;
  virtual void visitExternalContent(
      TilesetWalkerControl& control,
      Tile& tile,
      CesiumAsync::IAssetRequest& request,
      Tileset& externalTileset) = 0;
  virtual void visitUnknownContent(
      TilesetWalkerControl& control,
      Tile& tile,
      CesiumAsync::IAssetRequest& request) = 0;
};

class TilesetWalker {
public:
  void walkDepthFirst(Tileset& tileset, TilesetVisitor& visitor);
};

} // namespace Cesium3DTilesContent
