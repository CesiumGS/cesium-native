# Raster Overlays {#raster-overlays}

Cesium Native's raster overlays are georeferenced 2D images - perhaps consisting of trillions of pixels or more! - that are draped over the top of a [Tileset](\ref Cesium3DTilesSelection::Tileset). A classic example of a raster overlay is a satellite imagery layer. A `Tileset` can have multiple raster overlays, and they're usually alpha-blended together in a layered fashion. They can also be used for more sophisticated effects, however. For example, a raster overlay could represent a "mask" of where on Earth is land versus water, and that mask used in a custom shader or material to render waves in the water-covered areas.

The following raster overlay types are currently included in Cesium Native:

* [BingMapsRasterOverlay](\ref CesiumRasterOverlays::BingMapsRasterOverlay)
* [DebugColorizeTilesRasterOverlay](\ref CesiumRasterOverlays::DebugColorizeTilesRasterOverlay)
* [IonRasterOverlay](\ref CesiumRasterOverlays::IonRasterOverlay)
* [TileMapServiceRasterOverlay](\ref CesiumRasterOverlays::TileMapServiceRasterOverlay)
* [UrlTemplateRasterOverlay](\ref CesiumRasterOverlays::UrlTemplateRasterOverlay)
* [WebMapServiceRasterOverlay](\ref CesiumRasterOverlays::WebMapServiceRasterOverlay)
* [WebMapTileServiceRasterOverlay](\ref CesiumRasterOverlays::WebMapTileServiceRasterOverlay)

To add a raster overlay to a `Tileset`, construct an instance of the appropriate class and add it to the [RasterOverlayCollection](\ref Cesium3DTilesSelection::RasterOverlayCollection) returned by [Tileset::getOverlays](\ref Cesium3DTilesSelection::Tileset::getOverlays). See the reference documentation for each overlay for details about how to configure that overlay type.

For more information about `RasterOverlays`, see the following topics:

* \subpage implementing-a-new-raster-overlay-type - How to implement a new kind of `RasterOverlay`.
* \subpage how-raster-overlays-work - How Cesium Native's Raster Overlay system works under-the-hood.
