# How Raster Overlays Work {#how-raster-overlays-work}

This topic explains how Cesium Native's raster overlay system is implemented.

## Texture Coordinates {#texture-coordinates}

When rendered, raster overlay images are applied to geometry tiles just like any other texture: they're sampled in a pixel/fragment shader according to texture coordinates associated with every vertex in the geometry.

## Determining the raster overlay rectangle

## Target Screen Pixels

## Missing Tiles and Using Parent Textures
