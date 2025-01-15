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

## Tile Content Loading

It is important to understand the distinction between a [Tile](@ref Cesium3DTilesSelection::Tile) and its _content_. In the 3D Tiles specification, a [Tile](https://github.com/CesiumGS/3d-tiles/blob/main/specification/PropertiesReference_3dtiles.adoc#tile) is a node in the tileset's bounding-volume hierarchy (BVH), and has a bounding volume, a transform, a geometric error value, and more. Tiles are arranged in a tree, so every `Tile` has a parent `Tile` (except the root) and zero or more children. Tiles also have a `content.uri` property which points to the externally-stored _content_ for the tile. This external content is usually some sort of renderable object, such as a glTF model or a legacy container format like [b3dm](https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Batched3DModel/README.adoc#tileformats-batched3dmodel-batched-3d-model). It can also be a further subtree of the BVH rooted at this node, which is known as an [external tileset](https://github.com/CesiumGS/3d-tiles/tree/main/specification#core-external-tilesets).

Because _content_ is by far the largest part of a tile, and the most time-consuming to load, Cesium Native aims to only load it when when it is needed. The loading happens through a small state machine maintained in the `_loadState` property of each tile. The possible load states are captured in the [TileLoadState](@ref Cesium3DTilesSelection::TileLoadState) enumeration.

@mermaid{tile-load-states}

A `Tile` starts in the `Unloaded` state. A `Tile` in this state is added to the `_workerThreadLoadQueue` the first time it is visited during the selection algorithm. Any tiles that remain in this queue at the end of the selection (that is, that are not [kicked](#kicking)) are prioritized for loading as described in the [next section](#load-prioritization).

The number of tiles that may load simultaneously is controlled by the [TilesetOptions::maximumSimultaneousTileLoads](\ref Cesium3DTilesSelection::TilesetOptions::maximumSimultaneousTileLoads) property. We can picture tile loading as a swimming pool with `maximumSimultaneousTileLoads` swim lanes. The highest priority tiles jump in the pool, each in their own lane, and race for the other end. When a tile reaches the other side (finishes asynchronous loading), the next highest priority tile can jump in the pool in that now-unoccupied lane and start swimming. Multiple tiles can never share a swim lane.

The swim across the pool includes all of the steps of the tile loading process that do not need to happen on the main thread, including:

* Initiating an HTTP request for the tile content and receiving the response.
* Parsing the content from the response, such as parsing the glTF or external tileset JSON.
* Decoding compressed geometry and textures.
* Generating extra data needed for rendering, such as normals and raster overlay texture coordinates.

When a tile jumps in the pool, its `TileLoadState` is changed to `ContentLoading`. When it reaches the other end, the state is changed to `ContentLoaded`.

The next time the selection algorithm runs, and it sees a tile in the `ContentLoaded` state, the tile is added to the `_mainThreadLoadQueue`. Just like with the `_workerThreadLoadQueue`, tiles may be kicked from this queue as the selection algorithm proceeds, and those that remain are ranked by priority. This time, though, the loading happens synchronously, on the same thread that is running the selection algorithm. To avoid monopolizing too much main thread time, which could have a severe impact on interactivity, the highest priority tiles from this queue are processed only until the [TilesetOptions::mainThreadLoadingTimeLimit](\ref Cesium3DTilesSelection::TilesetOptions::mainThreadLoadingTimeLimit) has elapsed. Additional tiles will need to wait until the next frame.

The primary task that is completed during main thread loading is to call [IPrepareRendererResources::prepareInMainThread](\ref Cesium3DTilesSelection::IPrepareRendererResources::prepareInMainThread). See [Rendering 3D Tiles](#rendering-3d-tiles) for details.

Once a tile has completed its main-thread loading, it enters the `Done` state and is ready to be rendered.

If something goes wrong during the `ContentLoading` state, such as an HTTP error because the tile's content URL is invalid or the server is down, the tile enters the `Failed` state. Failed tiles will usualy show up as missing data or "holes" in the model. Content loads can also fail temporarily, such as when an access token expires and needs to be refreshed. Such tiles will transition to the `FailedTemporarily` state, and the async loading process will be restarted the next time the tile is needed.

Finally, there is the `Unloading` state. When a tile is no longer needed, it usually transitions directly and synchronously from the `ContentLoaded` or `Done` state to the `Unloaded` state. However, if the tile being unloaded happens at the same time to be the source of an active raster overlay "upsampling" operation, then unloading its content would lead to undefined behavior. Instead, we only unload its renderer resources (by calling [IPrepareRendererResources::free](\ref Cesium3DTilesSelection::IPrepareRendererResources::free)) but do _not_ delete its content. We put it in the `Unloading` state to mark that this has been done, and transition it to the `Unloaded` state when it is safe to do so. For more details, see the [pull request](https://github.com/CesiumGS/cesium-native/pull/554) where this mechanism was added.

## Load Prioritization {#load-prioritization}

Tiles that need to be loaded are prioritized so that the most important tiles are loaded first. Load priority consists of a priority _group_ plus a priority _value_ within that group. The group is chosen according to the reason that this tile is needed by the selection algorithm. The groups are as follows:

* `Preload` - Low priority tiles that aren't needed right now, but are being preloaded for the future.
* `Normal` - Medium priority tiles that are needed to render the current view at the appropriate level-of-detail.
* `Urgent` - High priority tiles whose absence is causing extra detail to be rendered in the scene, potentially creating a performance problem and aliasing artifacts.

Within the group, a tile with a lower priority value will be loaded before a tile with a higher priority value. The priority value is computed as follows:

```
(1.0 - dot(tileDirection, cameraDirection)) * distance
```

Where `distance` is the distance from the camera to the closest point on the tile, `tileDirection` is the unit vector from the camera to the center of the tile's bounding volume, and `cameraDirection` is the look direction of of the camera. The idea is that tiles that are near the center of the screen and closer to the camera are loaded first.

## Forbid Holes {#forbid-holes}

With the default settings, the tile selection algorithm prioritizes getting the needed detail to the screen as quickly as possible. The downside of this approach is that it can sometimes lead to "holes" - or parts of the model that are visibly missing - when the camera moves. This happens when the camera movement reveals a part of the model that wasn't previously visible, and the tiles necessary to show that part of the model are not yet loaded.

This default behavior can be changed by setting [TilesetOptions::forbidHoles](\ref Cesium3DTilesSelection::TilesetOptions::forbidHoles) to `true`. When holes are forbidden, loading will take longer, because some extra tiles will need to be loaded in order to fill the potential holes, and camera movement may still reveal areas of lower detail, but it will never reveal a part of the model that is missing entirely. This may offer a better user experience for many applications.

_Forbid Holes_ mode operates via a small change in `_visitTileIfNeeded`. Normally, when a tile is culled, we either don't load it at all, or we load with it with the lower `Preload` [priority](#load-prioritization). But when holes are forbidden, a culled tile is instead loaded with `Normal` priority, and it is also represented in the `TraversalDetails` returned from the method. This means that the subtree containing this tile will be [kicked](#kicking) if this tile is not yet loaded. This guarantees the subtree will not be rendered until this tile is loaded, which in turn guarantees that if the camera is moved, so that this tile suddenly becomes visible, it will be possible to render it immediately. There will not be a hole.

## Unconditionally-Refined Tiles

A tile that is "unconditionally refined" will always be _REFINED_, it will never be _RENDERED_. We can think of such a tile as having infinite geometric and screen-space error. Unconditionally-refined tiles are used in the following situations:

1. The `_pRootTile` held by the `TilesetContentManager`. This is root tile of the entire bounding-volume hierarchy. For a standard 3D Tiles `tileset.json` tileset, the root tile defined in the `tileset.json` is the single child of this `_pRootTile`.
2. Tiles whose "content" is an [external tileset](https://github.com/CesiumGS/3d-tiles/tree/main/specification#core-external-tilesets).
3. Tiles that have a geometric error that is higher than their parent's.

In all three cases, the tile flag is set with a call to [Tile::setUnconditionallyRefine](\ref Cesium3DTilesSelection::Tile::setUnconditionallyRefine).

Consider a tileset which has a root tile with some renderable content, and four children. Three of the children have normal renderable content as well, but the forth is an external tileset. This means that it provides more nodes for the bounding-volume hierarchy, but it does not have any renderable content itself. Even once that fourth tile is done loading, we can't render it. If we did, we would end up creating a visible hole in the tileset that would not be filled until the children of the tile finished loading as well.

Normally, a tile, no matter how large its screen-space error, can be rendered in some cases, such as when other tiles are not loaded yet. While we can conceptually think of unconditionally-refined tiles as simply having very large error, the handling of them within the selection algorithm goes a bit deeper, in order to ensure they are never rendered.

To start, [Tile::isRenderable](\ref Cesium3DTilesSelection::Tile::isRenderable) will return false for an unconditionally-refined tile. This will ensure the tile is [Kicked](#kicking) and thus not rendered.

> [!note]
> There is one exception where `isRenderable` will return true for an unconditionally-refined tile: when that tile also does not have any children, and never will. It would be an unusual - perhaps buggy! - tileset that has such a situation, but when it does occur, we must allow the tile to render so that its sibilings, if any, may render.

Also, when the children of an unconditionally-refined tile are kicked out of the render list, those tiles will _not_ also be kicked out of the load queue, even if the [Loading Descendant Limit](#loading-descendant-limit) is exceeded. Because the unconditionally-refined tile will never become renderable, failing to load its children would result in the entire subtree never becoming renderable.

Finally, when [Forbid Holes](#forbid-holes) is enabled, `_visitTileIfNeeded` will always visit unconditionally-refined tiles, even if they're culled. This is necessary because the non-renderable, unconditionally-refined tile would otherwise block renderable siblings from rendering, too. By visiting the unconditionally-refined tile, we allow its children to load, and thereby allow the subtree to become renderable.

## TilesetContentLoader {#tileset-content-loader}

The process of loading content and children for a tile is delegated to a pluggable interface called [TilesetContentLoader](\ref Cesium3DTilesSelection::TilesetContentLoader). This means that the Cesium Native 3D Tiles selection algorithm is not limited to 3D Tiles. Anything that can be portrayed in Cesium Native's 3D Tiles and glTF object model can be loaded, selected, and rendered by Cesium Native. Every `Tile` is associated with a loader, and that loader is used to load that `Tile`'s content. Child `Tiles` may use the same or a different loader.

The following `TilesetContentLoader` types are currently provided:

* `TilesetJsonLoader` - The standard loader for explicit 3D Tiles based on `tileset.json`. Individual tile content is loaded via [GltfConverters](\ref Cesium3DTilesContent::GltfConverters).
* `CesiumIonTilesetLoader` - Loads a 3D Tiles asset or `layer.json` / `quantized-mesh-1.0` terrain asset from [Cesium ion](https://cesium.com/platform/cesium-ion/), by delegating to one of the other loaders as appropriate. Automatically handles refreshing the token when it expires.
* `LayerJsonTerrainLoader` - Loads terrain described by a `layer.json` and individual terrain tiles in `quantized-mesh-1.0` format.
* `ImplicitQuadtreeLoader` - Loads a 3D Tiles 1.1 implicit quadtree.
* `ImplicitOctreeLoader` - Loads a 3D Tiles 1.1 implicit octree.
* `EllipsoidTilesetLoader` - Generates tiles on-the-fly by tessellating an ellipsoid, such as the WGS84 ellipsoid. Does not load any data from the disk or network.

Other loaders can be added by users of the library.

## Implicit Tilesets {#implicit-tilesets}

The tile selection algorithm selects tiles from an explicit representation of the bounding-volume hierarchy. Every tile in the tileset is represented as a [Tile](\ref Cesium3DTilesSelection::Tile) instance. Starting with 3D Tiles 1.1, the bounding-volume hierarchy may instead be defined _implicitly_ using [Implicit Tiling](https://github.com/CesiumGS/3d-tiles/tree/main/specification#core-implicit-tiling). This is much more efficient representation when the bounding-volume hierarchy has a uniform subdivision structure.

> [!note]
> The older `layer.json` / [quantized-mesh-1.0](https://github.com/CesiumGS/quantized-mesh) terrain format also uses a form of implicit tiling.

Cesium Native supports implicit tiling by lazily transforming the implicit representation into an explicit one as individual tiles are needed. This happens in the `TilesetContentManager::createLatentChildrenIfNecessary` method, called for each tile near the top of `_visitTileIfNeeded`. This method attempts to create explicit tile instances for the implicitly-defined children of the current tile by invoking [TilesetContentLoader::createTileChildren](\ref Cesium3DTilesSelection::TilesetContentLoader::createTileChildren). Thus, the `TilesetContentLoader` interface is not only responsible for loading tile content, it is also responsible for creating additional `Tile` instances in the bounding-volume hierarchy as needed.

Implicit [loaders](#tileset-content-loader), such as `ImplicitQuadtreeLoader`, `ImplicitOctreeLoader`, and `LayerJsonTerrainLoader`, implement this method by determining in their own way whether this tile has any children, and creating them if it does. In some cases, extra asynchronous work, like downloading subtree availability files, may be necessary to determine if children exist. In that case, the `createTileChildren` will return [TileLoadResultState::RetryLater](\ref Cesium3DTilesSelection::TileLoadResultState::RetryLater) to signal that children may exist, but they can't be created yet. The selection algorithm will try again next frame if the tile's children are still needed.

Currently, a `Tile` instance, once created, will not be destroyed until the entire [Tileset](\ref Cesium3DTilesSelection::Tileset) is destroyed. This is true for `Tile` instances created explicitly from `tileset.json` as well as `Tile` instances created lazily by the implicit loaders. This is convenient because we don't need to worry about a `Tile` instance vanishing unexpectedly, but it can cause a slow increase in memory usage over time.

> [!note]
> The above refers to `Tile` instances, _not_ their content. Content is unloaded when it is no longer needed. This is important because content is by far the largest portion of a tile.

## Additional Topics Not Yet Covered {#additional-topics}

Here are some additional selection algorithm topics that are not yet covered here, but should be in the future:

* Occlusion Culling
* Additive Refinement
