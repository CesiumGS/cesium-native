# 3D Tiles Selection Algorithm {#selection-algorithm-details}

In [Rendering 3D Tiles](#rendering-3d-tiles), we described how Cesium Native's support for 3D Tiles rendering can be integrated into an application. Here we go deeper, describing how [Tileset::updateView](@ref Cesium3DTilesSelection::Tileset::updateView) decides which tiles to load and render. This will primarily be of interest to developers improving Cesium Native itself, or perhaps to users who want a deep understanding in order to best optimize the available [TilesetOptions](@ref Cesium3DTilesSelection::TilesetOptions) for their use-case. It is assumed that the reader has familiarity with the [3D Tiles Specification](https://github.com/CesiumGS/3d-tiles/blob/main/specification/README.adoc).

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

@mermaid{tileset-culling-flowchart}

In general, a tile should be visited if it cannot be culled. A tile can be culled when any of these are true:

1. It is explicitly excluded by the application via the [ITileExcluder](@ref Cesium3DTilesSelection::ITileExcluder) interface.
2. It is outside the frustums of all views.
3. It is so far away relative to the camera height that it can be considered "obscured by fog", in all views.

A tile that is culled via (1) will never be visited. For cases (2) and (3), a culled tile may still be visited if `TilesetOptions` [enableFrustumCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFrustumCulling) or [enableFogCulling](@ref Cesium3DTilesSelection::TilesetOptions::enableFogCulling), respectively, are set to `false`.

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

A critical goal of the tile selection algorithm is to prevent visible detail from disappearing with camera movements.

When the camera moves, a new section of the model might be exposed that wasn't previously visible. If the tiles for this new section are not loaded yet, then we have no choice but to leave that section of the model blank. This is a case of detail not _appearing_ right away when we move the camera. While this is not ideal, and we try to avoid it when we can, detail that was already visible _disappearing_ is much worse.

> [!note]
> See [Forbid Holes](#forbid-holes) for details of an optional mode that will ensure that "the tiles for this new section are not loaded yet" can never happen, at the cost of additional loading time.

Detail would disappear very frequently if the tile selection algorithm were as simple as described so far. Imagine we're looking at a model, and everything is loaded and looking great. Now imagine that the user:

* _Zooms Out_: We suddenly want to render less detailed tiles in the same location. A simple selection algorithm would hide the detailed tiles immediately, before the less-detailed tiles are loaded. This would cause a part of the model that used to be visible to suddenly vanish, only to reappear a moment later after the new tiles were loaded. This would be extremely distracting for our users. Fortunately, this does not happen because of the [Ancestor Meets SSE](#ancestor-meets-sse) mechanism.
* _Zooms In_: More detailed tiles would suddenly be required in the same location. Once again, a simple selection algorithm would immediately hide the less detailed tiles, only to show the more detailed ones a moment later after they load. Fortunately, this does not happen because of the [Kicking](#kicking) mechanism.

There is a simple solution to both of these problems, but Cesium Native doesn't use it. The simple solution is to arrange the selection algorithm so that a tile can only be _REFINED_ if all of its child tiles are already loaded. This is easy, and avoids the problems described in this section. The reason that Cesium Native doesn't use it is because it makes for drastically slower loading.

Imagine a tileset organized as a quadtree, so each tile has four children. We're zoomed in close so that four tiles from level 15 of the quadtree would ideally be rendered. If we load those four tiles, we can render the scene from the current camera view, and it will look great. However, if we enforce the rule that "a tile can only be REFINED if its children are all loaded", then we would have to not only load those four tiles at level 15, we would also have to load at least four tiles at level 14, at least four more tiles at level 13, and so on all the way to the root of the tileset. Rather than loading four tiles to render this scene, we would need to load more like 60 tiles! The situation just described is a worst-case scenario, but on average, a _lot_ more tiles would need to be loaded.

## Ancestor Meets SSE {#ancestor-meets-sse}

The "Ancestor Meets SSE" mechanism is a feature of the tile selection algorithm that ensures detail does not disappear when the user moves away from the model. It does this by sometimes choosing to _REFINE_ a tile even though its SSE is less than the maximum, which would normally trigger a _RENDER_ instead. It is named for the `ancestorMeetsSse` flag that is set in `_visitTile` when we detect this case.

After `_visitTile` determines that it would like to _RENDER_ the current tile because it meets the required SSE, it does an additional check by calling `mustContinueRefiningToDeeperTiles`. If this function returns `true`, then the `ancestorMeetsSse` flag will be set and this tile will be _REFINED_, instead of _RENDERED_ as originally planned.

`mustContinueRefiningToDeeperTiles` returns `true` when the tile was _REFINED_ in the _previous_ frame AND it is not currently renderable. In other words, if descendant tiles were rendering last frame, and they can't be replaced by this tile yet because this tile isn't loaded, then we should continue giving those descendants the option to render.

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

The _Kicking_ mechanism is a feature of the tile selection algorithm that ensures detail does not disappear when the user moves closer to the model. It does this by discarding a subtree _after_ that subtree has been traversed and some of the tiles in it have been selected for rendering.

As mentioned previously in [Selecting Loaded Tiles](#selecting-loaded-tiles), it's very important when zooming closer to a model that the tile selection algorithm not hide the currently-shown, lower-detail tiles before the higher-detail tiles are loaded and ready to render. If it did, part of the model would momentarily blink out of existence when zooming in, which would be very distracting to the user. So, when deciding whether to _RENDER_ or _REFINE_ a tile, we may sometimes need to _RENDER_ even when the [Screen-Space Error](#screen-space-error) indicates we should _REFINE_. Specifically, this is the case when both of the following conditions are true:

1. Any selected tile in the subtree is not yet renderable.
2. None of the selected tiles in the subtree were rendered last frame.

Condition (1) is important because, once all of the subtree tiles are renderable, we want to render those instead of the current tile. Condition (2) is important because, if any of the subtree tiles were rendered last frame, and we stopped rendering them this frame, that would cause the scene to suddenly become less detailed while zooming in, which would also be very distracting to the user.

When these two conditions are met for a tile's descendants, then we should not render _any_ of those descendants, and instead render this single tile. This operation is known as _Kicking_. We must, however, continue loading the selected descendants, so that we can eventually render the proper tiles for the desired level-of-detail.

We decide whether a _Kick_ is necessary by first traversing the tile's children normally, as in a _REFINE_. Calling `_visitTileIfNeeded` on each child tile returns a `TraversalDetails` instance with three fields:

* `allAreRenderable`: `true` if every selected tile is renderable. `false` if even one selected tile is not yet renderable.
* `anyWereRenderedLastFrame`: `true` if any selected tile was rendered last frame. `false` if none of the selected tiles were rendered last frame.
* `notYetRenderableCount`: The number of selected tiles for which `isRenderable` is false.

The `TraversalDetails` for all children are combined in the intuitive way: `allAreRenderable` values are combined with a boolean AND, `anyWereRenderedLastFrame` values are combined with a boolean OR, and `notYetRenderableCount` values are combined by summing.

Now, we decide whether a _Kick_ is necessary by looking at the combined `allAreRenderable` and `anyWereRenderedLastFrame` properties. If both are false, we _Kick_!

The _Kick_ itself happens very efficiently. Before visiting this tile's children, we note the number of tiles in the render list. Visiting children will only add more tiles to this list, not remove or change any of the existing ones. So, to _Kick_, we:

1. Walk through all of the tiles added to the render list while visiting children and mark them _Kicked_ by calling [kick](@ref Cesium3DTilesSelection::TileSelectionState::kick) on their [TileSelectionState](@ref Cesium3DTilesSelection::TileSelectionState).
2. Remove all of these added tiles from the render list.
3. Add this tile to the render list.

When tiles are removed from the render list in favor of the current tile, they are also no longer represented in the `allAreRenderable` and `anyWereRenderedLastFrame` flags returned by `_visitTile` for the current tile. However, they _are_ represented in the current tile's `notYetRenderableCount`. This is important for computing the `loadingDescendantLimit`, as explained in the next section.

## Loading Descendant Limit {#loading-descendant-limit}

The [Kicking](#kicking) mechanism normally kicks tiles out of the render list, but _not_ out of the loading queues. As mentioned previously, we need those tiles to load so that we can eventually stop _Kicking_ them and start rendering them instead. However, there is one case where tiles are kicked out of the load queues, too.

After we decide to _Kick_ descendant tiles, we check whether the total `notYetRenderableCount` for all of the descendants is greater than the [loadingDescendantLimit](@ref Cesium3DTilesSelection::TilesetOptions::loadingDescendantLimit) expressed in `TilesetOptions`. If it is, that is when we will remove all of the descendant tiles from the load queues, too.

The purpose of this mechanism is to give the user feedback during the load process. Without it, an application starting out with a zoomed-in view, especially one that requires a lot of tiles such as a horizon view, would tend to show no tiles at all for awhile, waiting for all of the tiles necessary for the view to load. Then, once all tiles were loaded, they would appear, at full detail, all at once. This may not give the best user experience.

The `loadingDescendantLimit` works like a heuristic for deciding when intermediate, lower-detail tiles should be loaded and shown first, in order to give the user feedback that the load process is happening. The algorithm is happy to wait for a number of tiles to load before showing a more detailed subtree. This is the purpose of the _Kicking_ mechanism. But if the number of tiles it would have to wait for is too large - larger than `loadingDescendantLimit` - then it instead won't wait for any of them. The entire subtree load queue will be thrown out, and only the current tile will be loaded.

> [!note]
> When tiles are removed from the load queues due to exceeding the `loadingDescendantLimit`, they are also removed from representation in the `notYetRenderableCount` field. This is unlike with a normal _Kick_, where the _Kicked_ tiles are still represented in the `notYetRenderableCount`.

Once the current tile finishes loading and is rendered, only then will the tiles deeper in the subtree be given an opportunity to load and render. This ensures that the user sees the model sooner, at the cost of loading more tiles. The idea is to strike a tunable balance between user feedback and loading efficiency.

## Additional Topics Not Yet Covered {#additional-topics}

Here are some additional selection algorithm topics that are not yet covered here, but should be in the future:

* Load prioritization
* Forbid Holes <!--! \anchor forbid-holes !-->
* Unconditionally-Refined Tiles
* Occlusion Culling
* External Tilesets and Implicit Tiles
* Additive Refinement
