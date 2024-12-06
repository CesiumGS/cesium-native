# 3D Tiles Selection Algorithm {#selection-algorithm-details}

In [Rendering 3D Tiles](#rendering-3d-tiles), we described how Cesium Native's support for 3D Tiles rendering can be integrated into an application. Here we go deeper, describing how [Tileset::updateView](@ref Cesium3DTilesSelection::Tileset::updateView) decides which tiles to load and render. This will primarily be of interest to developers working on improving Cesium Native itself, or perhaps to users who want a deep understanding in order to best optimize the available [TilesetOptions](@ref Cesium3DTilesSelection::TilesetOptions) for their use-case. It is assumed that the reader has familiarity with the [3D Tiles Specification](https://github.com/CesiumGS/3d-tiles/blob/main/specification/README.adoc).

## High-Level Overview {#high-level-overview}

@mermaid{tileset-traversal-flowchart}

At a high level, `updateView` works by traversing the 3D Tiles tileset's bounding-volume hierarchy (BVH) in depth-first order. This is done using recursion, starting with a call to the private `_visitTileIfNeeded` method and passing it the root tile in the BVH. `_visitTileIfNeeded` checks to see if the tile needs to be [visited](#should-visit). The most important reason a tile would not need to be visited is if it's outside the view frustum of _all_ of the [ViewStates](@ref Cesium3DTilesSelection::ViewState) given to `updateView`. Such a tile is not visible, so we do not need to consider it further and traversal of this branch of the BVH stops here.

For tiles that _do_ need to be visited, `_visitTileIfNeeded` calls `_visitTile`.

The most important job of `_visitTile` is to decide whether the current tile - which has already been deemed visible - meets the required [screen-space error (SSE)](#screen-space-error) in all views. If the algorithm considers the current level-of-detail (LOD) of this part of the model, as represented by this tile, to be sufficient for the current view(s), then the tile is _rendered_. The tile is added to the [tilesToRenderThisFrame](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesToRenderThisFrame) and traversal of this branch of the BVH stops here. A tile without any children is always deemed to meet the required SSE.

If a tile that is selected for rendering is not yet loaded, it is queued for loading.

If the current tile does not provide sufficient detail for the current view(s), then `_visitTile` will instead _refine_, which means the algorithm will consider rendering this tile's children instead of this tile. It will call `_visitVisibleChildrenNearToFar`, which will recursively call `_visitTileIfNeeded` on this tile's children.

At a high level, this is all there is to the Cesium Native tile selection algorithm. As we look closer, though, a lot of important details come into focus. The details are covered in the sections below.

## Should this Tile be Visited? {#should-visit}

In the [High-Level Overview](#high-level-overview), we mentioned that `_visitTileIfNeeded` determines whether a tile should be visited before calling `_visitTile`. How does it decide this?

In general, a tile should be visited if it cannot be culled. A tile can be culled when any of these are true:

1. It is outside the frustums of all views.
2. It is so far away relative to the camera height that it can be considered "obscured by fog", in all views.
3. It is explicitly excluded by the application via the [ITileExcluder](@ref Cesium3DTilesSelection::ITileExcluder) interface.

A tile that is culled via (3) will never be visited, as you might expect.

For cases (1) and (2), a culled tile may still be visited if [enableFrustumCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFrustumCulling) or [enableFogCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFogCulling), respectively, in `TilesetOptions` is set to `false`.

When both of these options are disabled, tiles covering the entire model will be selected; there won't be any pieces of it missing once loading completes. Some parts will still be at a lower level-of-detail than other parts, however.

<!-- Mention unconditional refinement and forbid holes corner cases here? Or in their own sections? -->

## Screen-Space Error {#screen-space-error}

`_visitTile` decides whether to _render_ or _refine_ a tile based on its screen-space error (SSE). SSE is an estimate of how "wrong" a given tile will look when rendered, expressed as a distance in pixels on the screen. We can compute SSE as a function of several properties of a tile and a `ViewState`.

Every 3D Tiles tile has a [geometricError](https://github.com/CesiumGS/3d-tiles/tree/main/specification#geometric-error) property, which expresses the error, in meters, of the tile's representation of its part of the model compared to the original, full-detail model.

Tiles also have a [bounding volume](https://github.com/CesiumGS/3d-tiles/tree/main/specification#bounding-volumes) that contains all of the tile's triangles and other renderable content.

`Tileset` computes SSE separately for each `ViewState`, and then uses the _largest_ screen-space error computed for any `ViewState` to drive the refinement decision. It begins by computing the distance (squared) from the `ViewState`'s position to the closest point on the tile's bounding volume using [computeDistanceSquaredToBoundingVolume](@ref Cesium3DTilesSelection::ViewState::computeDistanceSquaredToBoundingVolume). If the `ViewState` is inside the bounding volume, the distance is treated as zero, and the SSE is effectively infinite. When the viewer is outside the bounding volume, though, we can project the tile's geometric error to the screen by calling [computeScreenSpaceError](@ref Cesium3DTilesSelection::ViewState::computeScreenSpaceError) and passing the tile's distance and geometric error.

Once we know the largest SSE of this tile as viewed from any `ViewState`, we can decide whether to refine this tile by comparing it against the [maximumScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::maximumScreenSpaceError) property in `TilesetOptions`. If the tile's SSE is greater than or equal to the maximum SSE, the tile will be refined in order to reduce the error on the screen. If the tile's SSE is less than the maximum SSE, it means this tile has sufficient detail for all views, so we can render it and we don't need to visit any of this tile's children.

This basic process always works as described, but the `maximumScreenSpaceError` that the tile SSE is compared against can vary based on some `TilesetOptions`.

Normally, as described in [Should this Tile be Visited?](#should-visit), tiles that are culled by the view-frustum or by fog are not visited at all. However, if either `enableFrustumCulling` or `enableFogCulling` is set to `false`, then these tiles will be visited and selected. And in that case, the question becomes: what level-of-detail should they be rendered at?

The answer is that refinement decisions for these tiles work the same way as for regular tiles, but are based on the value of the [culledScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::culledScreenSpaceError) instead of `maximumScreenSpaceError`. This allows us to use a lower LOD for tiles outside the view frustum or far off in the foggy distance. At an extreme, we can set the [enforceCulledScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::enforceCulledScreenSpaceError) property to `false` if we want to _never_ refine these tiles. This is equivalent to setting `culledScreenSpaceError` to a very large value. The non-visible tiles are selected, so all parts of the model will be represented at some level-of-detail, but the non-visible parts will use the lowest LOD that they can without having a hole in the model.

## Load Priority

## Kicking

## Forbid Holes

## Unconditionally-Refined Tiles

## Occlusion Culling

## External Tilesets and Implicit Tiles

## Additive Refinement

In the previous sections, we've assumed [replacement refinement](https://github.com/CesiumGS/3d-tiles/tree/main/specification#replacement), which is the more common case. A 3D Tiles tile may instead use [additive refinement](https://github.com/CesiumGS/3d-tiles/tree/main/specification#additive). This means that when the algorithm "refines" the tile, the content of the child tiles is added to the content of the parent tile, rather than replacing it.
