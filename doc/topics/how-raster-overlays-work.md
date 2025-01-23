# How Raster Overlays Work {#how-raster-overlays-work}

This topic explains how Cesium Native's raster overlay system is implemented.

> [!note]
> In the explanation below, we distinguish between a "geometry tile", which is an instance of [Tile](\ref Cesium3DTilesSelection::Tile) from a 3D Tiles or quantized-mesh-1.0 tileset, and a "raster overlay tile" which is an instance of [RasterOverlayTile](\ref CesiumRasterOverlays::RasterOverlayTile) that represents a raster overlay image applied to a geometry tile. While a `Tile` may have textures, too, a `RasterOverlayTile` never has geometry.

## Determining the raster overlay rectangle

In most cases, a `RasterOverlayTile` is mapped to a `Tile` when that geometry tile first transitions out of the _Unloaded_ state and starts its loading process. This happens with a call to the [RastedMappedTo3DTile::mapOverlayToTile](\ref Cesium3DTilesSelection::RasterMappedTo3DTile::mapOverlayToTile) static method. The first job of this method is to determine a bounding rectangle for this geometry tile, expressed in the coordinates and projection of the raster overlay.

If the renderable content for this tile were already loaded, this process would be simple. We would simply compute the projected coordinates of each vertex in the tile's model, and form a bounding rectangle from the minimum and maximum of these. Unfortunately, waiting until the model vertices are available is inefficient. It requires that we completely download, parse, and decode the tile's model before we can even start the requests for the raster overlay images. Ideally, we would be able to request the tile content and the raster overlay images simultaneously, which would significantly reduce latency.

To that end, we try to determine an accurate bounding rectangle for the geometry tile from the geometry tile's 3D bounding volume whenever we can. It's extremely important that the bounding rectangle be accurate, though. If we estimate a significantly larger rectangle than the geometry tile actually covers, we'll end up unnecessarily loading more raster overlay data than we actually need, eliminating the efficiency advantage we were hoping to gain. So `mapOverlayToTile` will only map actual raster overlay tiles to unloaded geometry tiles when it is sure it can determine an accurate rectangle. In all other cases, it instead adds a "placeholder" raster overlay tile, which will be turned into a real one later, after the geometry tile is loaded.

The current implementation can only determine an accurate rectangle for tiles with a [region](https://github.com/CesiumGS/3d-tiles/tree/main/specification#core-region) bounding volume. The reasoning is as follows:

1. Cesium Native currently only supports raster overlays with a [Geographic](\ref CesiumGeospatial::GeographicProjection) or [Web Mercator](\ref CesiumGeospatial::WebMercatorProjection) projection.
2. Both of these projections are aligned to longitude/latitude bounds. A constant X or Y coordinate in either projection corresponds to a constant longitude or latitude.
3. Therefore, as long as the 3D bounding region is accurate, the 2D bounding rectangle will be as well.

For all other bounding volumes, while we could attempt to estimate a bounding rectangle from the bounding volume, we would not have high confidence in the estimate. So, instead, we accept the latency and load the geometry tile first.

## Texture Coordinates {#texture-coordinates}

When rendered, raster overlay images are applied to geometry tiles just like any other texture: they're sampled in a pixel/fragment shader according to texture coordinates associated with every vertex in the geometry. Cesium Native generates one or more sets of overlay texture coordinates for every geometry tile that has raster overlays.

In fact, it generates a set of texture coordinate for each unique projection used by raster overlays. We need texture coordinates per projection because the raster overlay texture sampling also serves to unproject the overlay images from the raster overlay's projected coordinate system to the 3D world coordinates.

This unprojection is not perfect. Texture coordinates are defined at vertices and are linearly interpolated over triangles. This linear interpolation may be noticeably different from the result we would get if we actually computed the projected coordinates at a given world position within the triangle. A more correct solution would be to evaluate the projection in the shader or material. This would be less performant, however, and most map projections are difficult to evaluate accurately in the single-precision floating point arithmetic that is commonly available on GPUs. Most importantly, unprojecting with texture coordinates and sampling is good enough in all but pathological cases. Geospatial models tend to have vertices that are close enough together to avoid noticeable artifacts caused by inaccurate unprojection in between them.

Texture coordinates are computed using [RasterOverlayUtilities::createRasterOverlayTextureCoordinates](\ref CesiumRasterOverlays::RasterOverlayUtilities::createRasterOverlayTextureCoordinates), which is called from a worker thread during the geometry tile loading process.

## Target Screen Pixels

The pixel resolution used for the raster overlay texture applied to a given geometry tile is controlled by the overlay tile's "target screen pixels". This is a parameter that Cesium Native passes to the `RasterOverlayTileProvider` when it requests an overlay image. Cesium Native computes this quantity by calling [RasterOverlayUtilities::computeDesiredScreenPixels](\ref CesiumRasterOverlays::RasterOverlayUtilities::computeDesiredScreenPixels). The return value is a 2D vector, with the X and Y components representing the target number of pixels in the projected X and Y directions, respectively. See the reference documentation for that method, as well as the comments in the implementation, for further details.

We can think of this as the maximum pixel size of the geometry tile on the screen, just before moving a little closer to it would cause a switch to a higher level-of-detail. This is a function of the geometry tile's bounding box, as well as its geometric error and the tileset's maximum screen-space error. For leaf tiles with a geometric error of zero, we compute this quantity as if the tile had a geometric error that is half of its parent's.

The "Target Screen Pixels" is not usually used directly as the size of the raster overlay texture, however, but it is used to compute it. First, it is divided by the [maximumScreenSpaceError](\ref CesiumRasterOverlays::RasterOverlayOptions::maximumScreenSpaceError) configured on the `RasterOverlay`. Then, it is clamped to the [maximumTextureSize](\ref CesiumRasterOverlays::RasterOverlayOptions::maximumTextureSize). This formulation makes raster overlay detail shown on the screen a function of its own maximum screen-space error property, and largely independent of the maximum screen-space error configured on the `Tileset`. We can cut the geometry detail in half by changing [TilesetOptions::maximumScreenSpaceError](\ref Cesium3DTilesSelection::TilesetOptions::maximumScreenSpaceError) from 16.0 to 32.0, and this will not affect the sharpness of the raster overlays.

This formulation does, however, mean that changing `TilesetOptions::maximumScreenSpaceError` requires remapping raster overlaps to geometry tiles, which is usually accomplished by reloading the `Tileset` entirely.
