# Rendering 3D Tiles {#rendering-3d-tiles}

One of the main reasons to integrate Cesium Native into an application is to add support for rendering [3D Tiles](https://github.com/CesiumGS/3d-tiles). This guide explains how that can be done.

A 3D Tiles [Tileset](@ref Cesium3DTilesSelection::Tileset) is a potentially massive 3D model - such as the entire Earth! - broken up into a bounding-volume hierarchy (BVH) of small pieces, called tiles. The main challenge of rendering a 3D Tiles tileset lies in deciding which tiles need to be loaded and rendered each frame, employing view-dependent culling and level-of-detail techniques. Fortunately, Cesium Native takes care of these details.

It's important to understand that Cesium Native doesn't actually do any rendering itself, though. Instead, it provides each tile in the form of an in-memory [glTF](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) model, and it's up to your integration to do the actual rendering. These glTF models are rendering-ready static meshes, so rendering them is relatively straightforward in most environments.

## Sequence Walkthrough {#sequence-walkthrough}

In order to understand the pieces that you will need to implement in order to complete your integration, let's walk through a sequence of render frames and show what happens along the way. In this diagram:

* `Application` is the application you're developing.
* `Tileset` is the Cesium Native [Tileset](@ref Cesium3DTilesSelection::Tileset) class.
* `IAssetAccessor` is Cesium Native's interface to download assets, i.e., files on the file system or a web server. You must implement this interface.
* `IPrepareRendererResources` is Cesium Native's interface to create "renderer" resources on-demand for the glTF models that it provides. For example, if you're integrating Cesium Native into a game engine, the renderer resource might be an instance of the game engine's static mesh class. You must implement this interface as well.

@mermaid{tileset-sequence-diagram-frame1}

In Frame 1, your application calls [updateView](@ref Cesium3DTilesSelection::Tileset::updateView) on the `Tileset`. It passes in all of the [ViewStates](@ref Cesium3DTilesSelection::ViewState) from which the tileset is currently being viewed. A `ViewState` includes a position, a look direction, a camera "up" direction, a viewport width and height in pixels, and horizontal and vertical field-of-view angles. This is all the information that Cesium Native needs in order to decide which subset of the model is visible, and what level-of-detail is needed for each part. You'll likely create a `ViewState` from each camera in your scene.

In our example, based on the `ViewStates`, Cesium Native selects tiles A and B as being needed for rendering. The details of this process are described in the [3D Tiles Selection Algorithm](@ref selection-algorithm-details), but aren't important for now. In Frame 1, no tiles are loaded yet, so `Tileset` calls [IAssetAccessor::get](@ref CesiumAsync::IAssetAccessor::get) to initiate the download of these two tiles. These downloads happen asynchronously via the [AsyncSystem](#async-system); Cesium Native doesn't wait for them to complete before continuing.

@mermaid{tileset-sequence-diagram-frame2}

In Frame 2, your application calls `updateView` again, as it will every frame. If any views have changed since last frame, you should provide the new views, and Cesium Native will adapt accordingly.

At the start of the `updateView`, the `Tileset` happens to receive the asynchronous content for Tile A that it requested in Frame 1. It's entirely possible that this download could take multiple frames, or that it could complete in between calls to `updateView`, rather than completing for the next frame as we've shown here.

`updateView` runs the selection algorithm again, and once again selects Tiles A and B. `Tileset` is still waiting for content for Tile B, so it can't do anything more there. However, it's already received the content for Tile A, so it can start the next part of the process: [prepareInLoadThread](@ref Cesium3DTilesSelection::IPrepareRendererResources::prepareInLoadThread). As the name implies, this method is invoked from a background worker thread (dispatched using [ITaskProcessor](@ref CesiumAsync::ITaskProcessor)) and gives your integration its first opportunity to do renderer resource preparation for Tile A's glTF model.

> [!note]
> In most applications, it's possible to do some of the work necessary to prepare a static mesh for rendering in a background thread, but at least a small part of that work must be done in the application's "main" thread. That is why the glTF preparation process is split into two parts: `prepareInLoadThread`, which we've just seen, and `prepareInMainThread`, which we'll see in the next frame. Applications are free to divide the work between these two methods however they see fit. Some applications may not use one or the other of them at all.

@mermaid{tileset-sequence-diagram-frame3}

In Frame 3, two asynchronous operations begun in previous frames resolve (complete successfully). The first is the `prepareInLoadThread` for Tile A, and the second is the download of Tile B's content. `Tileset` runs the selection algorithm again, and happens to come up with the same answer, selecting tiles A and B.

Now that `Tileset` has content for Tile B, it can initiate the `prepareInLoadThread` for Tile B, as it did for Tile A last frame. And now that the `prepareInLoadThread` has resolved for Tile A, `Tileset` can call [prepareInMainThread](@ref Cesium3DTilesSelection::IPrepareRendererResources::prepareInMainThread) for Tile A. Unlike `prepareInLoadThread`, `prepareInMainThread` is a synchronous operation that must complete and return before the work of `updateView` can continue. For this reason, it's important to keep `prepareInMainThread` as fast as possible!

> [!note]
> By "main thread", we mean the thread that called `updateView`. This does not necessarily have to be the thread that your application considers to be the main one. However, you must ensure that multiple threads do not access `Tileset` simultaneously. In Debug builds of Cesium Native, assertions will prevent you from calling `updateView` from different threads even if you ensure only one thread at a time is doing so, because this usually indicates a mistake and the potential for a subtle race condition.

`prepareInLoadThread` and `prepareInMainThread` should ensure that the renderer resources they create are initially _not_ visible in the scene. This is important because some tiles are pre-loaded, before they're actually needed for rendering.

With Tile A selected and its loading complete, the `Tileset` will return it from `updateView` as one of the [tilesToRenderThisFrame](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesToRenderThisFrame). Your application must look through the returned set of tiles and ensure that each is visible (rendered). Similarly, tiles in the [tilesFadingOut](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesFadingOut) should be hidden or faded out.

@mermaid{tileset-sequence-diagram-frame4}

In Frame 4, the `prepareInLoadThread` initiated for Tile B in Frame 3 resolves. When `Tileset` runs the selection algorithm, it learns that, because a view has moved, only Tile B is selected now. It calls `prepareInMainThread` on Tile B to prepare it for rendering.

`Tileset` also calls [free](@ref Cesium3DTilesSelection::IPrepareRendererResources::free) to release the renderer resources that were created for Tile A in `prepareInLoadThread` and `prepareInMainThread`. Once the renderer resources are freed, the `Tileset` will release the glTF tile content as well. In practice, this `free` may or may not actually happen this frame. Cesium Native keeps some tiles around, up to a [maximumCachedBytes](@ref Cesium3DTilesSelection::TilesetOptions::maximumCachedBytes) specified in [TilesetOptions](@ref Cesium3DTilesSelection::TilesetOptions), in case they are needed again soon. Cesium Native will ensure that the tiles it calls `free` on are not currently visible in the scene.

With that out of the way, `Tileset` returns the new set of `tilesToRenderThisFrame` (B). It will also include Tile A in the `tilesFadingOut`.

## Implementing 3D Tiles Rendering {#implement-3d-tiles-rendering}

As illustrated above, integrating 3D Tiles rendering in your application requires the following:

1. Implement [ITaskProcessor](@ref CesiumAsync::ITaskProcessor) to run jobs in background threads, preferably using a thread pool or task graph.
2. Implement [IAssetAccessor](@ref CesiumAsync::IAssetAccessor) to download resources from whatever sources your application needs to load 3D Tiles from. A possible approach is to use [libcurl](https://curl.se/libcurl/), but many applications already include file and HTTP support.
3. Implement [IPrepareRendererResources](@ref Cesium3DTilesSelection::IPrepareRendererResources) to create meshes and textures for your application from the in-memory glTF representations provided by Cesium Native.
4. When constructing a [Tileset](@ref Cesium3DTilesSelection::Tileset), pass in instances of the three implementations above as part of the [TilesetExternals](@ref Cesium3DTilesSelection::TilesetExternals).
5. Call [updateView](@ref Cesium3DTilesSelection::Tileset::updateView) on each `Tileset` each frame. Show the already-created models identified in [tilesToRenderThisFrame](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesToRenderThisFrame) and hide the ones in [tilesFadingOut](@ref Cesium3DTilesSelection::ViewUpdateResult::tilesFadingOut).

In practice, the bulk of the work is usually in item (3). While glTF is a an efficient format for rendering, implementing a robust and performant pathway from a glTF to your applications rendering system can involve a fair bit of work. It's usually possible, however, to get the basics working relatively quickly.
