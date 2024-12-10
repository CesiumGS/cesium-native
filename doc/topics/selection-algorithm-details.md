# 3D Tiles Selection Algorithm {#selection-algorithm-details}

In [Rendering 3D Tiles](#rendering-3d-tiles), we described how Cesium Native's support for 3D Tiles rendering can be integrated into an application. Here we go deeper, describing how [Tileset::updateView](@ref Cesium3DTilesSelection::Tileset::updateView) decides which tiles to load and render. This will primarily be of interest to developers working on improving Cesium Native itself, or perhaps to users who want a deep understanding in order to best optimize the available [TilesetOptions](@ref Cesium3DTilesSelection::TilesetOptions) for their use-case. It is assumed that the reader has familiarity with the [3D Tiles Specification](https://github.com/CesiumGS/3d-tiles/blob/main/specification/README.adoc).

## High-Level Overview {#high-level-overview}

@mermaid{tileset-traversal-flowchart}

At a high level, `updateView` works by traversing the 3D Tiles tileset's bounding-volume hierarchy (BVH) in depth-first order. This is done using recursion, starting with a call to the private `_visitTileIfNeeded` method and passing it the root tile in the BVH. `_visitTileIfNeeded` checks to see if the tile needs to be [visited](#culling). The most important reason a tile would not need to be visited is if it's outside the view frustum of _all_ of the [ViewStates](@ref Cesium3DTilesSelection::ViewState) given to `updateView`. Such a tile is not visible, so we do not need to consider it further and traversal of this branch of the BVH stops here.

For tiles that _do_ need to be visited, `_visitTileIfNeeded` calls `_visitTile`.

The most important job of `_visitTile` is to decide whether the current tile - which has already been deemed visible - meets the required [screen-space error (SSE)](#screen-space-error) in all views. If the algorithm considers the current level-of-detail (LOD) of this part of the model, as represented by this tile, to be sufficient for the current view(s), then the tile is _RENDERED_. The tile is added to the [tilesToRenderThisFrame](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesToRenderThisFrame) and traversal of this branch of the BVH stops here. A tile without any children is always deemed to meet the required SSE.

If a tile that is selected for rendering is not yet loaded, it is queued for loading.

If the current tile does not provide sufficient detail for the current view(s), then `_visitTile` will instead _REFINE_, which means the algorithm will consider rendering this tile's children instead of this tile. It will call `_visitVisibleChildrenNearToFar`, which will recursively call `_visitTileIfNeeded` on this tile's children.

At a high level, this is all there is to the Cesium Native tile selection algorithm. As we look closer, though, a lot of important details come into focus. The details are covered in the sections below.

## Culling {#culling}

In the [High-Level Overview](#high-level-overview), we mentioned that `_visitTileIfNeeded` determines whether a tile should be visited before calling `_visitTile`. How does it decide this?

In general, a tile should be visited if it cannot be culled. A tile can be culled when any of these are true:

1. It is outside the frustums of all views.
2. It is so far away relative to the camera height that it can be considered "obscured by fog", in all views.
3. It is explicitly excluded by the application via the [ITileExcluder](@ref Cesium3DTilesSelection::ITileExcluder) interface.

A tile that is culled via (3) will never be visited. For cases (1) and (2), a culled tile may still be visited if `TilesetOptions` [enableFrustumCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFrustumCulling) or [enableFogCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFogCulling), respectively, are set to `false`.

When both of these options are `false`, `_visitTile` will be called for all tiles that aren't explicitly excluded, so tiles covering the entire model will be selected. There will not be any pieces of the model missing once loading completes. Some parts may still be at a lower level-of-detail than other parts, however.

<!-- Mention unconditional refinement and forbid holes corner cases here? Or in their own sections? -->

## Screen-Space Error {#screen-space-error}

`_visitTile` decides whether to _RENDER_ or _REFINE_ a tile based on its screen-space error (SSE). SSE is an estimate of how "wrong" a given tile will look when rendered, expressed as a distance in pixels on the screen. We can compute SSE as a function of several properties of a tile and a `ViewState`.

Every 3D Tiles tile has a [geometricError](https://github.com/CesiumGS/3d-tiles/tree/main/specification#geometric-error) property, which expresses the error, in meters, of the tile's representation of its part of the model compared to the original, full-detail model.

Tiles also have a [bounding volume](https://github.com/CesiumGS/3d-tiles/tree/main/specification#bounding-volumes) that contains all of the tile's triangles and other renderable content.

`Tileset` computes SSE separately for each `ViewState`, and then uses the _largest_ screen-space error computed for any `ViewState` to drive the refinement decision. It begins by computing the distance (squared) from the `ViewState`'s position to the closest point on the tile's bounding volume using [computeDistanceSquaredToBoundingVolume](@ref Cesium3DTilesSelection::ViewState::computeDistanceSquaredToBoundingVolume). If the `ViewState` is inside the bounding volume, the distance is treated as zero, and the SSE is effectively infinite. When the viewer is outside the bounding volume, though, we can project the tile's geometric error to the screen by calling [computeScreenSpaceError](@ref Cesium3DTilesSelection::ViewState::computeScreenSpaceError) and passing the tile's distance and geometric error.

Once we know the largest SSE of this tile as viewed from any `ViewState`, we can decide whether to refine this tile by comparing it against the [maximumScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::maximumScreenSpaceError) property in `TilesetOptions`. If the tile's SSE is greater than or equal to the maximum SSE, the tile will be refined in order to reduce the error on the screen. If the tile's SSE is less than the maximum SSE, it means this tile has sufficient detail for all views, so we can render it and we don't need to visit any of this tile's children.

This basic process always works as described, but the maximum SSE that the tile SSE is compared against can vary for culled tiles.

Normally, as described in [Culling](#culling), tiles that are culled by the view-frustum or by fog are not visited at all. However, if either `enableFrustumCulling` or `enableFogCulling` is set to `false`, then these tiles will be visited and selected. And in that case, the question becomes: what level-of-detail should they be rendered at?

The answer is that refinement decisions for these tiles work the same way as for regular tiles, but are based on the value of the [culledScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::culledScreenSpaceError) property instead of `maximumScreenSpaceError`. This allows us to use a lower LOD (larger maximum SSE value) for tiles outside the view frustum or far off in the foggy distance. At an extreme, we can set the [enforceCulledScreenSpaceError](@ref Cesium3DTilesSelection::TilesetOptions::enforceCulledScreenSpaceError) property to `false` if we want to _never_ refine these tiles. This is equivalent to setting `culledScreenSpaceError` to a very large value. The non-visible tiles are selected, so all parts of the model will be represented at some level-of-detail, but the non-visible parts will use the lowest LOD that they can without having a hole in the model.

## Selecting Loaded Tiles {#selecting-loaded-tiles}

3D Tiles are usually streamed over the network. The entire tileset is rarely located on the local computer, and almost never loaded entirely into memory. Thus, it's inevitable that we will sometimes want to render tiles that haven't been loaded yet.

A critical goal of the tile selection algorithm is to prevent visible detail from disappearing with camera movements. When the camera moves, a new section of the model might be exposed that wasn't previously visible. If the tiles for this new section are not loaded yet, then we have no choice but to leave that section of the model blank. This is a case of detail not _appearing_ right away when we move the camera. While it's not ideal, and we try to avoid it when we can, detail _disappearing_ is much worse.

> [!note]
> See [Forbid Holes](#forbid-holes) for details of an optional mode that will ensure that "the tiles for this new section are not loaded yet" can never happen, at the cost of additional loading time.

Detail would disappear very frequently if the tile selection algorithm were as simple as described so far. Imagine we're looking at a model, and everything is loaded and looking great. Now imagine that the user:

* _Zooms Out_: We suddenly want to render less detailed tiles in the same location. A simple selection algorithm would hide the detailed tiles immediately, before the less-detailed tiles are loaded. This would cause a part of the model that used to be visible to suddenly vanish, only to reappear a moment later after the new tiles were loaded. This would be extremely distracting for our users. Fortunately, this does not happen because of the [Ancestor Meets SSE](#ancestor-meets-sse) mechanism.
* _Zooms In_: More detailed tiles would suddenly be required in the same location. Once again, a simple selection algorithm would immediately hide the less detailed tiles, only to show the more detailed ones a moment later after they load. Fortunately, this does not happen because of the [Kicking](#kicking) mechanism.

There is a simple solution to both of these problems, but Cesium Native doesn't use it. The simple solution is to arrange the selection algorithm so that a tile can only be _REFINED_ if all of its child tiles are already loaded. This is easy, and avoids the problems described in this section. The reason that Cesium Native doesn't use it is because it makes for drastically slower loading.

Imagine a tileset organized as a quadtree, so each tile has four children. We're zoomed in close so that four tiles from level 15 of the quadtree would ideally be rendered. If we load those four tiles, we can render the scene from the current camera view, and it will look great. However, if we enforce the "a tile can only be REFINED if its children are all loaded" rule, then we would have to not only load those four tiles at level 15, we would also have to load at least four tiles at level 14, at least four more tiles at level 13, and so on all the way to the root of the tileset. Rather than loading four tiles to render this scene, we would need to load more like 60 tiles! The scenario just described is a worst-case scenario, but on average, a _lot_ more tiles would need to be loaded.

## Ancestor Meets SSE {#ancestor-meets-sse}

The "Ancestor Meets SSE" mechanism is a feature of the tile selection algorithm that ensures detail does not disappear when the user moves away from the model. It does this by sometimes choosing to _REFINE_ a tile even though its SSE is less than the maximum, which would normally trigger a _RENDER_ instead.

The "depth" of a tile is the number of tiles we have to traverse to get to a particular tile, starting with the root tile. This concept isn't really used anywhere in the code, which is based on geometric error instead, but it's useful in the discussion that follows.

Imagine our user is zoomed in close to a tileset, so tiles with a depth of 15 are visible. Then they zoom out a little bit, so that, ideally, tiles with a depth of 14 are now visible in that space. But oh no: the level 14 tiles haven't been loaded yet! What do we do?

It might be tempting to do one of the following:

1. Show nothing at all in that space, until the depth 14 tiles are loaded.
2. Show the most detailed tiles with a depth <= 14 that we have available to fill that space.

Both of these options are not ideal because they result in detail that the user can see suddenly disappearing from the scene.

Instead, Cesium Native will continue to render the level 15 tiles until the less detailed tiles are loaded, and then it will switch to those. The mechanism that accomplishes this is a flag passed to `_visitTile` called `ancestorMeetsSse`.

<table>
  <tr>
    <th>Zooming out without the benefit of "Ancestor Meets SSE"</th>
    <th>"Ancestor Meets SSE" preserves current detail</th>
  </tr>
  <tr>
    <td>![Zooming out without the benefit of "Ancestor Meets SSE"](without-ancestor-meets-sse.gif)</td>
    <td>!["Ancestor Meets SSE" preserves current detail until lower detail is loaded](with-ancestor-meets-sse.gif)</td>
  </tr>
</table>



## Kicking {#kicking}


## Load Priority


## Forbid Holes {#forbid-holes}

## Unconditionally-Refined Tiles

## Occlusion Culling

## External Tilesets and Implicit Tiles

## Additive Refinement

In the previous sections, we've assumed [replacement refinement](https://github.com/CesiumGS/3d-tiles/tree/main/specification#replacement), which is the more common case. A 3D Tiles tile may instead use [additive refinement](https://github.com/CesiumGS/3d-tiles/tree/main/specification#additive). This means that when the algorithm "refines" the tile, the content of the child tiles is added to the content of the parent tile, rather than replacing it.
