# Implementing a new RasterOverlay Type {#implementing-a-new-raster-overlay-type}

Raster overlays are implemented by deriving from the [RasterOverlay](\ref CesiumRasterOverlays::RasterOverlay) abstract base class, so new ones can be easily added even from outside of Cesium Native. `RasterOverlay` has just a single pure-virtual method that must be implemented: [createTileProvider](\ref CesiumRasterOverlays::RasterOverlay::createTileProvider). This method [asynchronously](#async-system) produces an instance of a class derived from [RasterOverlayTileProvider](\ref CesiumRasterOverlays::RasterOverlayTileProvider).

A `RasterOverlayTileProvider` has a particular [Projection](\ref CesiumGeospatial::Projection), which is used to select or generate appropriate texture coordinates for this raster overlay, and a rectangle that the raster overlay covers, expressed in the coordinates of that map projection.

> [!note]
> In the explanation below, we distinguish between a "geometry tile", which is an instance of [Tile](\ref Cesium3DTilesSelection::Tile) from a 3D Tiles or quantized-mesh-1.0 tileset, and a "raster overlay tile" which is an instance of [RasterOverlayTile](\ref CesiumRasterOverlays::RasterOverlayTile) that represents a raster overlay image applied to a geometry tile. While a `Tile` may have textures, too, a `RasterOverlayTile` never has geometry.

While it's possible to derive a class from `RasterOverlayTileProvider` directly and implement the [loadTileImage](\ref CesiumRasterOverlays::RasterOverlayTileProvider::loadTileImage) method, there are two shortcuts available that often save a lot of implementation effort.

In the very common scenario where a raster overlay source is organized into a quadtree of tiles, and each tile can be downloaded from a web URL, we can implement `createTileProvider` to construct an instance of [UrlTemplateRasterOverlay](\ref CesiumRasterOverlays::UrlTemplateRasterOverlay) with the appropriate templatized URL and then call its `createTileProvider`:

\snippet{trimleft} ExamplesRasterOverlays.cpp use-url-template

If we need a little more control, or if the raster overlay images are not downloaded from web URLs, then we can derive a new class from [QuadtreeRasterOverlayTileProvider](\ref CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider) and create and return an instance of it from `createTileProvider`. This requires implementing the [loadQuadtreeTileImage](\ref CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider::loadQuadtreeTileImage) method, which is given a quadtree tile ID (level, x, and y) and must asynchronously return the image for that quadtree tile. `QuadtreeRasterOverlayTileProvider` itself will automatically figure out an appropriate quadtree level to use for ther raster overlay tile attached to a given geometry tile, and it will make multiple calls to `loadQuadtreeTileImage` as necessary to get all of the quadtree images that cover it. In the common case that multiple geometry tiles overlap a single raster overlay quadtree tile, a small cache ensures that raster overlay tiles are not requested more than is necessary.

If our raster overlay source is not arranged in a quadtree, however, we're left with the final option, which is deriving from `RasterOverlayTileProvider` directly. This requires implementing the [loadTileImage](\ref CesiumRasterOverlays::RasterOverlayTileProvider::loadTileImage) method. When Cesium Native calls this method, it passes a [RasterOverlayTile](\ref CesiumRasterOverlays::RasterOverlayTile) which captures the requirements for the raster overlay tile that covers this geometry tile:

* [Rectangle](\ref CesiumRasterOverlays::RasterOverlayTile::getRectangle): Describes the minimum rectangle that the provided image must cover, expressed in the provider's [Projection](\ref CesiumRasterOverlays::RasterOverlayTileProvider::getProjection). The returned image is allowed to be bigger than this, but the extra pixels will be wasted.
* [TargetScreenPixels](\ref CesiumRasterOverlays::RasterOverlayTile::getTargetScreenPixels): The number of pixels on the screen that the rectangle is expected to map to, just before the geometry tile switches to a higher level-of-detail. This is used to control how detailed the image will be.

In a typical implementation, the target screen pixels is divided by the raster overlay's configured [maximumScreenSpaceError](\ref CesiumRasterOverlays::RasterOverlayOptions::maximumScreenSpaceError) to determine the target number of pixels in the raster overlay image. Then, pixels are copied into the output image in order to completely fill the rectangle. The output image rectangle is usually larger than the `RasterOverlayTile` rectangle, and the number of pixels is slightly higher than the target number of pixels in the raster overlay image, because both must be rounded up in order to fully include partial pixels.

\image html raster-overlay-mapping.svg
