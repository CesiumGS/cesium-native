# 3D Tiles Selection Algorithm {#selection-algorithm-details}

In [Rendering 3D Tiles](#rendering-3d-tiles), we described how Cesium Native's support for 3D Tiles rendering can be integrated into an application. Here we go deeper, describing how [Tileset::updateView](@ref Cesium3DTilesSelection::Tileset::updateView) decides which tiles to load and render. This will primarily be of interest to developers working on improving Cesium Native itself, or perhaps to users who want a deep understanding in order to best optimize the available [TilesetOptions](@ref Cesium3DTilesSelection::TilesetOptions) for their use-case. It is assumed that the reader has familiarity with the [3D Tiles Specification](https://github.com/CesiumGS/3d-tiles/blob/main/specification/README.adoc).

## High Level Overview {#high-level-overview}

At a high level, `updateView` works by traversing the 3D Tiles tileset's bounding-volume hierarchy (BVH) in a depth-first order. This is done using recursion, starting with a call to the private `_visitTileIfNeeded` method and giving it the root tile in the BVH. `_visitTileIfNeeded` does some initial checks to determine if the tile needs to be traversed. The most important reason a tile might not need to be traversed is if it's outside the view frustum of _all_ of the [ViewStates](@ref Cesium3DTilesSelection::ViewState) given to `updateView`. Such a tile is not visible, so we do not need to consider it further.

For tiles that _do_ need to be traversed, `_visitTileIfNeeded` calls `_visitTile`.

The most important job of `_visitTile` is to decide whether the current tile - which has already been deemed visible - should be _rendered_ or _refined_.

## Screen-Space Error

## Culling

## Load Priority

## Kicking

## Forbid Holes

## Occlusion Culling
