# RasterOverlayTileProvider

Loads or creates a `RasterOverlayTile` to cover a given geometry tile.

The rest of the 3D Tiles engine calls `getTile`, giving it the exact rectangle that the returned tile must cover. The tile provider has a `Projection`, and the provided rectangle is expressed in that projection.

`getTile` returns a `RasterOverlayTile`, but does not immediately start loading it. The 3D Tiles engine calls `RasterOverlayTileProvider::loadTile` to kick off the loading process. `getTile` may return nullptr if the provider has no data within the given rectangle.

While the returned `RasterOverlayTile` is loading, the 3D Tiles engine will use the geometry from the parent geometry tile. The `RasterOverlayTileProvider` doesn't need to concern itself with this at all. It is handled by `RasterMappedTo3DTile`.

`RasterOverlayTileProvider` also, in general, does not need to do any caching. The 3D Tiles engine will only call `getTile` once per geometry tile.

`getTile` internally calls the polymorphic `loadTileImage`. Derived `RasterOverlayTileProvider` classes implement this method to kick off a request, if necessary, then decode the result and provide the decoded pixels as a `LoadedRasterOverlayImage`. All the lifecycle management is handled automatically by `RasterOverlayTileProvider`, so that derived classes only need to implement this one async method.

# QuadtreeRasterOverlayTileProvider

Derives from `RasterOveralyTileProvider` and provides an implementation for `RasterOverlayTileProvider::loadTileImage`. This implementation looks a bit like the old `mapRasterTilesToGeometryTile`. It figures out which quadtree tiles fall inside the rectangle passed to `loadTileImage`. Then, it starts an async load of each of these tiles and waits for them _all_ to either finish loading or fail to load. For the ones that fail to load, we try a parent tile, which may require more waiting. Eventually we have tiles that cover the entire rectangle, and we can blit them to the output texture. `loadTileImage` is an async method start to finish, so this is straightforward. It doesn't require any tricky lifetime management.
