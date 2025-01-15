# Raster Overlays {#raster-overlays}

Cesium Native's raster overlays are georeferenced 2D images - perhaps consisting of trillions of pixels or more! - that are draped over the top of a [Tileset](\ref Cesium3DTilesSelection::Tileset). A classic example of a raster overlay is a satellite imagery layer. A `Tileset` can have multiple raster overlays, and they're usually alpha-blended together in a layered fashion. They can also be used for more sophisticated effects, however. For example, a raster overlay could represent a "mask" of where on Earth is land versus water, and that mask used in a custom shader or material to render waves in the water-covered areas.

The following raster overlay types are currently included in Cesium Native:

* [BingMapsRasterOverlay](\ref CesiumRasterOverlays::BingMapsRasterOverlay)
* [DebugColorizeTilesRasterOverlay](\ref CesiumRasterOverlays::DebugColorizeTilesRasterOverlay)
* [IonRasterOverlay](\ref CesiumRasterOverlays::IonRasterOverlay)
* [WebMapServiceRasterOverlay](\ref CesiumRasterOverlays::WebMapServiceRasterOverlay)
* [WebMapTileServiceRasterOverlay](\ref CesiumRasterOverlays::WebMapTileServiceRasterOverlay)
* [TileMapServiceRasterOverlay](\ref CesiumRasterOverlays::TileMapServiceRasterOverlay)

To add a raster overlay to a `Tileset`, construct an instance of the appropriate class and add it to the [RasterOverlayCollection](\ref Cesium3DTilesSelection::RasterOverlayCollection) returned by [Tileset::getOverlays](\ref Cesium3DTilesSelection::Tileset::getOverlays). See the reference documentation for each overlay for details about how to configure that overlay type.

## Implementing a new RasterOverlay {#implementing-a-raster-overlay}

Raster overlays are implemented by deriving from the [RasterOverlay](\ref CesiumRasterOverlays::RasterOverlay) abstract base class, so new ones can be easily added even from outside of Cesium Native. `RasterOverlay` has just a single pure-virtual method that must be implemented: [createTileProvider](\ref CesiumRasterOverlays::RasterOverlay::createTileProvider). This method [asynchronously](#async-system) produces an instance of a class derived from [RasterOverlayTileProvider](\ref CesiumRasterOverlays::RasterOverlayTileProvider).

A `RasterOverlayTileProvider` has a particular [Projection](\ref CesiumGeospatial::Projection) and a rectangle that it covers, expressed in that projection.

Deriving a class from `RasterOverlayTileProvider`, in turn, requires implementing one more pure-virtual method: [loadTileImage](\ref CesiumRasterOverlays::RasterOverlayTileProvider::loadTileImage). This class is passed an instance of [RasterOverlayTile](\ref CesiumRasterOverlays::RasterOverlayTile) and is expected to asynchronously produce a [LoadedRasterOverlayImage](\ref CesiumRasterOverlays::LoadedRasterOverlayImage), containing the actual image data plus details...

## RasterOverlayTileProvider


The job of a `RasterOverlayTileProvider` is to create `RasterOverlayTile` instances on demand to cover each geometry tile.

Loads or creates a `RasterOverlayTile` to cover a given geometry tile.

The rest of the 3D Tiles engine calls `getTile`, giving it the exact rectangle that the returned tile must cover. The tile provider has a `Projection`, and the provided rectangle is expressed in that projection.

`getTile` returns a `RasterOverlayTile`, but does not immediately start loading it. The 3D Tiles engine calls `RasterOverlayTileProvider::loadTile` to kick off the loading process. `getTile` may return nullptr if the provider has no data within the given rectangle.

While the returned `RasterOverlayTile` is loading, the 3D Tiles engine will use the geometry from the parent geometry tile. The `RasterOverlayTileProvider` doesn't need to concern itself with this at all. It is handled by `RasterMappedTo3DTile`.

`RasterOverlayTileProvider` also, in general, does not need to do any caching. The 3D Tiles engine will only call `getTile` once per geometry tile.

`getTile` internally calls the polymorphic `loadTileImage`. Derived `RasterOverlayTileProvider` classes implement this method to kick off a request, if necessary, then decode the result and provide the decoded pixels as a `LoadedRasterOverlayImage`. All the lifecycle management is handled automatically by `RasterOverlayTileProvider`, so that derived classes only need to implement this one async method.

# QuadtreeRasterOverlayTileProvider

Derives from `RasterOveralyTileProvider` and provides an implementation for `RasterOverlayTileProvider::loadTileImage`. This implementation looks a bit like the old `mapRasterTilesToGeometryTile`. It figures out which quadtree tiles fall inside the rectangle passed to `loadTileImage`. Then, it starts an async load of each of these tiles and waits for them _all_ to either finish loading or fail to load. For the ones that fail to load, we try a parent tile, which may require more waiting. Eventually we have tiles that cover the entire rectangle, and we can blit them to the output texture. `loadTileImage` is an async method start to finish, so this is straightforward. It doesn't require any tricky lifetime management.
