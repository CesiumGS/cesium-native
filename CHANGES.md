# Change Log

### v0.25.0 - 2023-06-01

##### Additions :tada:

- Added `computeTransformationToAnotherLocal` method to `LocalHorizontalCoordinateSystem`.
- Added support for the `KHR_materials_variants` extension to the glTF reader and writer.
- Added `GunzipAssetAccessor`. It can decorate another asset accessor in order to automatically gunzip responses (if they're gzipped) even if they're missing the proper `Content-Encoding` header.

##### Fixes :wrench:

- On Tileset Load Failure, warning/error messages will always be logged even if the failure callback is set.
- Fixed a bug that caused meshes to be missing entirely when upsampled from a parent with `UNSIGNED_BYTE` indices.

### v0.24.0 - 2023-05-01

##### Additions :tada:

- `WebMapServiceRasterOverlay` now allows query parameters in the base URL when building GetCapabilities and GetMap requests.
- Added support for parsing implicit tilesets that conform to the 3D Tiles 1.1 Spec.

##### Fixes :wrench:
- Fixed various `libjpeg-turbo` build errors, including ones that occurred when building for iOS.

### v0.23.0 - 2023-04-03

##### Breaking Changes :mega:

- Removed `tilesLoadingLowPriority`, `tilesLoadingMediumPriority`, and `tilesLoadingHighPriority` from `ViewUpdateResult`. Use `workerThreadTileLoadQueueLength` and `mainThreadTileLoadQueueLength` instead.

##### Additions :tada:

- Added `getOrientedBoundingBoxFromBoundingVolume` to the `Cesium3DTilesSelection` namespace.
- Added `transform` and `toAxisAligned` methods to `OrientedBoundingBox`.
- Switched to `libjpeg-turbo` instead of `stb` for faster jpeg decoding.
- Added `getNumberOfTilesLoaded` method to `Tileset`.
- Changed how `TilesetOptions::forbidHoles` works so that it loads much more quickly, while still guaranteeing there are no holes in the tileset.
- Added `frameNumber` property to `ViewUpdateResult`.
- Added getters for the `stride` and `data` fields of `AccessorView`.
- Added `startNewFrame` method to `ITileExcluder`.
- Added `CreditSystem.setShowOnScreen` and `Tileset.setShowCreditsOnScreen` to allow on-screen credit rendering to be toggled at runtime.

##### Fixes :wrench:

- Fixed a bug that caused the `center` field of `AxisAlignedBox` to be incorrect.
- Fixed a bug that caused the main thread to sometimes load low-priority tiles before high-priority ones. This could result in much longer waits than necessary for a tileset's appropriate level-of-detail to be shown.
- Fixed a bug that prevented WebP and KTX2 textures from working in the common case where only the extension specified the `source` property, not the glTF's main `Texture` definition.

### v0.22.1 - 2023-03-06

##### Fixes :wrench:

- Fixed a crash that could occur when a batch table property had fewer values than the model had features.

### v0.22.0 - 2023-03-01

##### Breaking Changes :mega:

- Renamed `CesiumGeometry::AxisTransforms` to simply `Transforms`.
- Renamed `CesiumGeospatial::Transforms` to `GlobeTransforms`.

##### Additions :tada:

- Added `GlobeAnchor`, making it easy to define a coordinate system that anchors an object to the globe and maintains it as the object moves or as the local coordinate system it is defined in changes.
- Added support for loading tilesets with `pnts` content. Point clouds are converted to `glTF`s with a single `POINTS` primitive, while batch tables are converted to `EXT_feature_metadata`.
- Added `createTranslationRotationScaleMatrix` and `computeTranslationRotationScaleFromMatrix` methods to `CesiumGeometry::Transforms`.
- Added `CesiumUtility::AttributeCompression` for encoding and decoding vertex attributes in different formats.

##### Fixes :wrench:

- Fixed a bug that could cause holes to appear in a tileset, even with frustum culling disabled, when the tileset includes some empty tiles with a geometric error greater than their parent's.

### v0.21.3 - 2023-02-01

##### Fixes :wrench:

- Fixed a bug that could prevent loading in tilesets that are additively-refined and have external tilesets, such as Cesium OSM Buildings.
- Fixed a bug that could cause parent tiles to be incorrectly culled in tilesets with additive ("ADD") refinement. This could cause geometry to disappear when moving in closer, or fail to appear at all.
- When unloading tile content, raster overlay tiles are now detached from geometry tiles _before_ the geometry tile content is unloaded.
- Added missing `#include <string>` in generated glTF and 3D Tiles header files.
- Replaced `std::sprintf` with `std::snprintf`, fixing a warning-as-error in newer versions of Xcode.
- Upgraded tinyxml2 [from commit 1aeb57d26bc303d5cfa1a9ff2a331df7ba278656 to commit e05956094c27117f989d22f25b75633123d72a83](https://github.com/leethomason/tinyxml2/compare/1aeb57d26bc303d5cfa1a9ff2a331df7ba278656...e05956094c27117f989d22f25b75633123d72a83).

### v0.21.2 - 2022-12-09

##### Additions :tada:

- Added the ability to specify the endpoint URL of the Cesium ion API when constructing an `IonRasterOverlay`.

##### Fixes :wrench:

- Removed the logged warning about the use of the `gltfUpAxis` property in a 3D Tiles tileset.json. While not technically spec-compliant, this property is quite common and we are not going to remove support for it anytime soon.

### v0.21.1 - 2022-12-02

##### Fixes :wrench:

- Fixed a bug that could cause an assertion failure - and on rare occasions a more serious problem - when creating a tile provider for a `TileMapServiceRasterOverlay` or a `WebMapServiceRasterOverlay`.

### v0.21.0 - 2022-11-01

##### Breaking Changes :mega:

- On `IPrepareRendererResources`, the `image` parameter passed to `prepareRasterInLoadThread` and the `rasterTile` parameter passed to `prepareRasterInMainThread` are no longer const. These methods are now allowed to modify the parameters during load.
- `IPrepareRendererResources::prepareInLoadThread` now takes a `TileLoadResult` and returns a `Future<TileLoadResultAndRenderResources>`, allowing it to work asynchronously rather than just blocking a worker thread until it is finished.
- `RasterOverlay::createTileProvider` now takes the owner pointer as an `IntrusivePointer` instead of a raw pointer, and returns a future that resolves to a `RasterOverlay::CreateTileProviderResult`.

##### Additions :tada:

- Added `mainThreadLoadingTimeLimit` and `tileCacheUnloadTimeLimit` properties to `TilesetOptions`, allowing a limit to be placed on how much time is spent loading and unloading tiles per frame.
- Added `GltfReader::generateMipMaps` method.
- Added the `getImage` method to `RasterOverlayTile`.
- Added `LocalHorizontalCoordinateSystem`, which is used to create convenient right- or left-handeded coordinate systems with an origin at a point on the globe.

##### Fixes :wrench:

- Fixed a bug that could cause a crash when adding raster overlays to sparse tilesets and zooming close enough to cause them to be upsampled.

### v0.20.0 - 2022-10-03

##### Breaking Changes :mega:

- `TileRenderContent::lodTransitionPercentage` now always goes from 0.0 --> 1.0 regardless of if the tile is fading in or out.
- Added a new parameter to `IPrepareRendererResources::prepareInLoadThread`, `rendererOptions`,  to allow passing arbitrary data from the renderer.

##### Fixes :wrench:

- In `CesiumGltfWriter`, `accessor.byteOffset` and `bufferView.byteOffset` are no longer written if the value is 0. This fixes validation errors for accessors that don't have buffer views, e.g. attributes that are Draco compressed.
- Fixed a bug where failed tiles don't clean up any raster overlay tiles that are mapped to them, and therefore cannot be rendered as empty tiles.
- Fixed a bug that prevented access to Cesium Ion assets by using expired Access Tokens.

### v0.19.0 - 2022-09-01

##### Breaking Changes :mega:

- `RasterOverlayCollection` no longer accepts a `Tileset` in its constructor. Instead, it now accepts a `Tile::LoadedLinkList` and a `TilesetExternals`.
- Removed `TileContext`. It has been replaced by the `TilesetContentLoader` interface.
- Removed `TileContentFactory`. Instead, conversions of various types to glTF can be registered with `GltfConverters`.
- Removed `TileContentLoadInput`. It has been replaced by `TileLoadInput` and `TilesetContentLoader`.
- Removed `TileContentLoadResult`. It has been replaced by `TileContent`.
- Removed `TileContentLoader`. It has been replaced by `TilesetContentLoader` and `GltfConverters`.
- Removed `ImplicitTraversal`. It has been replaced by `TilesetContentLoader` and `GltfConverters`.
- Removed many methods from the `Cesium3DTilesSelection::Tileset` class: `getUrl()`, `getIonAssetID()`, `getIonAssetToken()`, `notifyTileStartLoading`, `notifyTileDoneLoading()`, `notifyTileUnloading()`, `loadTilesFromJson()`, `requestTileContent()`,  `requestAvailabilitySubtree()`, `addContext()`, and `getGltfUpAxis()`. Most of these were already not recommended for use outside of cesium-native.
- Removed many methods from the `Cesium3DTilesSelection::Tile` class: `getTileset()`, `getContext()`, `setContext()`, `getContent()`, `setEmptyContent()`, `getRendererResources()`, `setState()`, `loadContent()`, `processLoadedContent()`, `unloadContent()`, `update()`, and `markPermanentlyFailed()`. Most of these were already not recommended for use outside of cesium-native.

##### Additions :tada:

- Quantized-mesh terrain and implicit octree and quadtree tilesets can now skip levels-of-detail when traversing, so the correct detail is loaded more quickly.
- Added new options to `TilesetOptions` supporting smooth transitions between tiles at different levels-of-detail. A tile's transition percentage can be retrieved from `TileRenderContent::lodTransitionPercentage`.
- Added support for loading WebP images inside glTFs and raster overlays. WebP textures can be provided directly in a glTF texture or in the `EXT_texture_webp` extension.
- Added support for `KHR_texture_transform` to `CesiumGltf`, `CesiumGltfReader`, and `CesiumGltfWriter`
- `Tileset` can be constructed with a `TilesetContentLoader` and a root `Tile` for loading and rendering different 3D Tile-like formats or creating a procedural tileset.

##### Fixes :wrench:

- Fixed a bug where the Raster Overlay passed to the `loadErrorCallback` would not be the one that the user created, but instead an aggregated overlay that was created internally.

### v0.18.1 - 2022-08-04

##### Fixes :wrench:

- Fixed a bug in `SqliteCache` where the last access time of resources was not updated correctly, sometimes causing more recently used resources to be evicted from the cache before less recently used ones.

### v0.18.0 - 2022-08-01

##### Breaking Changes :mega:

- Removed support for 3D Tiles Next extensions in `TilesetWriter` and `TilesetReader` that have been promoted to core in 3D Tiles 1.1
  - [3DTILES_multiple_contents](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_multiple_contents)
  - [3DTILES_implicit_tiling](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_implicit_tiling)
  - [3DTILES_metadata](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_metadata)
  - [3DTILES_content_gltf](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_content_gltf)
- Removed the `getSupportsRasterOverlays` from `Tileset` because the property is no longer relevant now that all tilesets support raster overlays.

##### Additions :tada:

- Added support for [3D Tiles 1.1](https://github.com/CesiumGS/3d-tiles/pull/666) in `TilesetWriter` and `TilesetReader`.
- Added a `TileOcclusionRendererProxyPool` to `TilesetExternals`. If a renderer implements and provides this interface, the tile occlusion information is used to avoid refining parent tiles that are completely occluded, reducing the number of tiles loaded.
- `Tileset` can now estimate the percentage of the tiles for the current view that have been loaded by calling the `computeLoadProgress` method.
- Enabled loading Tile Map Service (TMS) URLs that do not have a file named "tilemapresource.xml", such as from GeoServer.
- Added support for Tile Map Service documents that use the "local" profile when the SRS is mercator or geodetic.

### v0.17.0 - 2022-07-01

##### Fixes :wrench:
- Fixed crash when parsing an empty copyright string in the glTF model.

### v0.16.0 - 2022-06-01

##### Additions :tada:

- Added option to the `RasterizedPolygonsOverlay` to invert the selection, so everything outside the polygons gets rasterized instead of inside.
- The `RasterizedPolygonsTileExcluder` excludes tiles outside the selection instead of inside when given an inverted `RasterizedPolygonsOverlay`.
- Tiles are now upsampled using the projection of the first raster overlay in the list with more detail.

##### Fixes :wrench:

- For consistency with CesiumJS and compatibility with third-party terrain tilers widely used in the community, the `bounds` property of the `layer.json` file of a quantized-mesh terrain tileset is now ignored, and the terrain is assumed to cover the entire globe.

### v0.15.2 - 2022-05-13

##### Fixes :wrench:

- Fixed a bug where upsampled quadtree tiles could have siblings with mismatching projections.

In addition to the above, this release updates the following third-party libraries used by cesium-native:
- `cpp-httplib` to v0.10.3 ([changes](https://github.com/yhirose/cpp-httplib/compare/c7486ead96dad647b9783941722b5944ac1aaefa...d73395e1dc652465fa9524266cd26ad57365491f))
- `draco` to v1.5.2 ([changes](https://github.com/google/draco/compare/9bf5d2e4833d445acc85eb95da42d715d3711c6f...bd1e8de7dd0596c2cbe5929cbe1f5d2257cd33db))
- `earcut` to v2.2.3 ([changes](https://github.com/mapbox/earcut.hpp/compare/6d18edf0ce046023a7cb55e69c4cd9ba90e2c716...b28acde132cdb8e0ef536a96ca7ada8a651f9169))
- `PicoSHA2` to commit `1677374f23352716fc52183255a40c1b8e1d53eb` ([changes](https://github.com/okdshin/PicoSHA2/compare/b699e6c900be6e00152db5a3d123c1db42ea13d0...1677374f23352716fc52183255a40c1b8e1d53eb))
- `rapidjson` to commit `fcb23c2dbf561ec0798529be4f66394d3e4996d8` ([changes](https://github.com/Tencent/rapidjson/compare/fd3dc29a5c2852df569e1ea81dbde2c412ac5051...fcb23c2dbf561ec0798529be4f66394d3e4996d8))
- `spdlog` to v1.10.0 ([changes](https://github.com/gabime/spdlog/compare/cbe9448650176797739dbab13961ef4c07f4290f...76fb40d95455f249bd70824ecfcae7a8f0930fa3))
- `stb` to commit `af1a5bc352164740c1cc1354942b1c6b72eacb8a` ([changes](https://github.com/nothings/stb/compare/b42009b3b9d4ca35bc703f5310eedc74f584be58...af1a5bc352164740c1cc1354942b1c6b72eacb8a))
- `uriparser` to v0.9.6 ([changes](https://github.com/uriparser/uriparser/compare/e8a338e0c65fd875a46067d711750e4c13e044e7...24df44b74753017acfaec4b3a30097a8a2ae1ae1))

### v0.15.1 - 2022-05-05

##### Fixes :wrench:

- Fixed a bug that could cause tiles in external tilesets to fail to load.

### v0.15.0 - 2022-05-02

##### Additions :tada:

- Improved the load performance when `TilesetOptions::forbidHoles` is enabled by only loading child tiles when their parent does not meet the necessary screen-space error requirement.
- Added support for loading availability metadata from quantized-mesh layer.json. Previously, only availability embedded in terrain tiles was used.
- Added support for quantized-mesh terrain tilesets that specify a parent layer.
- Added support for metadata from the `3DTILES_batch_table_hierarchy` extension.

##### Fixes :wrench:

- Fixed a bug that could cause the same tiles to be continually loaded and unloaded when `TilesetOptions::forbidHoles` was enabled.
- Fixed a bug that could sometimes cause tilesets to fail to show their full detail when making changes to raster overlays.
- Fixed a bug that could cause holes even with `TilesetOptions::forbidHoles` enabled, particularly when using external tilesets.
- Tiles will no longer be selected to render when they have no content and they have a higher "geometric error" than their parent. In previous versions, this situation could briefly lead to holes while the children of such tiles loaded.
- Fixed a bug where `IPrepareRendererResources::prepareInMainThread` was called on a `Tile` before that `Tile` was updated with loaded content.
- Fixed a bug where getting bad data from the SQLite request cache could cause a crash. If the SQLite database is corrupt, it will now be deleted and recreated.

### v0.14.1 - 2022-04-14

##### Fixes :wrench:

- Fixed a crash caused by using an aggregated overlay of `IonRasterOverlay` after it is freed.
- Fix a bug introduced in v0.14.0 that caused Tile Map Service (TMS) overlays from Cesium ion to fail to load.

### v0.14.0 - 2022-04-01

##### Breaking Changes :mega:

- Added a new parameter, `rendererOptions`, to `IPrepareRendererResources::prepareRasterInLoadThread`.
- Changed the type of Cesium ion asset IDs from `uint32_t` to `int64_t`.
- Various changes in the `Cesium3DTiles`, `Cesium3DTilesReader`, and `Cesium3DTilesWriter` namespaces to match the evolving 3D Tiles Next specifications.
- Removed `getTextureCoordinateIndex` from `FeatureIDTextureView` and `FeatureTexturePropertyView`. Use `getTextureCoordinateAttributeId` instead.

##### Additions :tada:

- Added `WebMapServiceRasterOverlay` to pull raster overlays from a WMS server.
- Added support for the following glTF extensions to `CesiumGltf`, `CesiumGltfReader`, and `CesiumGltfWriter`:
  - `EXT_instance_features`
  - `EXT_structural_metadata`
  - `MAXAR_mesh_variants`
- Added an in-memory cache for Cesium ion asset endpoint responses in order to avoid repeated requests.
- Added `ScopeGuard` class to automatically a execute function when exiting a scope.
- The glTF `copyright` property, if present, is now included in the credits that `Tileset` adds to the `CreditSystem`. If the `copyright` has multiple parts separate by semicolons, these are treated as separate credits.
- Credits reported by `CreditSystem::getCreditsToShowThisFrame` are now sorted based on the number of occurrences, with the most common credits first.
- `Tileset` and `RasterOverlay` credits can now be shown on the screen, rather than in a separate credit popup.
- Added `FeatureTexturePropertyView::getSwizzle` method.
- Added `IsMetadataArray` template to check if a type is a `MetadataArrayView`.
- Added a `rendererOptions` property to `RasterOverlayOptions` to pass arbitrary data to `prepareRasterInLoadThread`.
- Added `Uri::escape`.

##### Fixes :wrench:

- Fixed an issue that could lead to compilation failures when passing an lvalue reference to `Promise::resolve()`.
- Fixed upsampling for `EXT_feature_metadata` feature tables.
- Fixed a bug that could cause the size of external images to be accounted for incorrectly when tracking the number of bytes loaded for caching purposes.
- Fixed a bug that prevented tiles from loading when "Forbid Holes" option was enabled.

### v0.13.0 - 2022-03-01

##### Breaking Changes :mega:

- Renamed constants in `CesiumUtility::Math` to use PascalCase instead of SCREAMING_SNAKE_CASE.

##### Additions :tada:

- Added support for the `CESIUM_RTC` and `KHR_texture_basisu` glTF extensions.
- Added support for 3D Tiles that do not have a geometric error, improving compatibility with tilesets that don't quite match the 3D Tiles spec.
- Exposed the Cesium ion endpoint URL as a parameter on tilesets and raster overlays.
- `TilesetOptions` and `RasterOverlayOptions` each have a new option to report which compressed textured formats are supported on the client platform. Ideal formats amongst the available ones are picked for each KTX2 texture that is later encountered.
- The `ImageCesium` class nows convey which GPU pixel compression format (if any) is used. This informs what to expect in the image's pixel buffer.
- The `ImageCesium` class can now contain pre-computed mipmaps, if they exist. In that case, all the mips will be in the pixel buffer and the delineation between each mip will be described in `ImageCesium::mipPositions`.
- Tileset content with the known file extensions ".gltf", ".glb", and ".terrain" can now be loaded even if the Content-Type is incorrect. This is especially helpful for loading tilesets from `file:` URLs.
- Created tighter fitting bounding volumes for terrain tiles by excluding skirt vertices.

##### Fixes :wrench:

- Fixed bug that could cause properties types in a B3DM Batch Table to be deduced incorrectly, leading to a crash when accessing property values.
- Fixed a bug where implicit tiles were not receiving the root transform and so could sometimes end up in the wrong place.

### v0.12.0 - 2022-02-01

##### Breaking Changes :mega:

- Renamed `IAssetAccessor::requestAsset` to `get`.
- Renamed `IAssetAccessor::post` to `request` and added a new parameter in the second position to specify the HTTP verb to use.
- `Token` in `CesiumIonClient` has been updated to match Cesium ion's v2 REST API endpoint, so several fields have been renamed. The `tokens` method also now returns future that resolves to a `TokenList` instead of a plain vector of `Token` instances.
- Renamed `GltfReader::readModel`, `ModelReaderResult`, and `ReadModelOptions` to `GltfReader::readGltf`, `GltfReaderResult`, and `GltfReaderOptions` respectively.
- Removed `writeModelAsEmbeddedBytes`, `writeModelAndExternalFiles`, `WriteModelResult`, `WriteModelOptions`, and `WriteGLTFCallback`. Use `GltfWriter::writeGltf`, `GltfWriter::writeGlb`, `GltfWriterResult`, and `GltfWriterOptions` instead.

##### Additions :tada:

- Added `TilesetWriterOptions` for serializing tileset JSON.
- Added support for the following extensions in `GltfWriter` and `GltfReader`:
  - [KHR_materials_unlit](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_unlit)
  - [EXT_mesh_gpu_instancing](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor/EXT_mesh_gpu_instancing)
  - [EXT_meshopt_compression](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor/EXT_meshopt_compression)
  - [EXT_mesh_features](https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_mesh_features)
  - [CESIUM_tile_edges](https://github.com/CesiumGS/glTF/pull/47)
- Added support for the following extensions in `TilesetWriter` and `TilesetReader`:
  - [3DTILES_multiple_contents](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_multiple_contents)
  - [3DTILES_implicit_tiling](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_implicit_tiling)
  - [3DTILES_metadata](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_metadata)
- Added `SubtreeWriter` and `SubtreeReader` for serializing and deserializing the subtree format in [3DTILES_implicit_tiling](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_implicit_tiling).
- Added `SchemaWriter` and `SchemaReader` for serializing and deserializing schemas in [EXT_mesh_features](https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_mesh_features) and [3DTILES_metadata](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_metadata).
- Added `hasExtension` to `ExtensibleObject`.
- Added `CESIUM_TESTS_ENABLED` option to the build system.
- Added support in the JSON reader for reading doubles with no fractional value as integers.
- Added case-insensitive comparison for Cesium 3D Tiles "refine" property values.
- Added new capabilities to `Connection` in `CesiumIonClient`:
  - The `tokens` method now uses the v2 service endpoint and allows a number of options to be specified.
  - Added a `token` method to allow details of a single token to be retrieved.
  - Added `nextPage` and `previousPage` methods to allow paging through tokens.
  - Added `modifyToken` method.
  - Added static `getIdFromToken` method to obtain a token ID from a given token value.
- Added `loadErrorCallback` to `TilesetOptions` and `RasterOverlayOptions`. This callback is invoked when the `Tileset` or `RasterOverlay` encounter a load error, allowing the error to be handled by application code.
- Enable `IntrusivePointer<T>` to be converted to `IntrusivePointer<U>` if U is a base class of T.

##### Fixes :wrench:

- Fixes a bug where `notifyTileDoneLoading` was not called when encountering Ion responses that can't be parsed.
- Fixed a bug that prevented a continuation attached to a `SharedFuture` from returning a `Future` itself.
- Fixed incorrect child subtree index calculation in implicit tiles.
- Fixed `computeDistanceSquaredToPosition` in `BoundingSphere`.

### v0.11.0 - 2022-01-03

##### Breaking Changes :mega:

- The `CesiumGltfReader` project now uses the `CesiumGltfReader` namespace instead of the `CesiumGltf` namespace.
- The `CesiumGltfWriter` project now uses the `CesiumGltfWriter` namespace instead of the `CesiumGltf` namespace.
- The `Cesium3DTilesReader` project now uses the `Cesium3DTilesReader` namespace instead of the `Cesium3DTiles` namespace.

##### Additions :tada:

- Added `Cesium3DTilesWriter` library.

##### Fixes :wrench:

- Fixed a bug in `QuadtreeRasterOverlayTileProvider` that caused incorrect level-of-detail selection for overlays that use a global (or otherwise large) tiling scheme but have non-global (or otherwise smaller) coverage.

### v0.10.0 - 2021-12-01

##### Breaking Changes :mega:

- `QuadtreeRasterOverlayTileProvider::computeLevelFromGeometricError` has been removed. `computeLevelFromTargetScreenPixels` may be useful as a replacement.
- The constructor of `RasterOverlayTileProvider` now requires a coverage rectangle.
- `RasterOverlayTileProvider::getTile` now takes a `targetScreenPixels` instead of a `targetGeometricError`.
- The constructor of `RasterMappedTo3DTile` now requires a texture coordinate index.
- The constructor of `RasterOverlayTile` now takes a `targetScreenPixels` instead of a `targetGeometricError`. And the corresponding `getTargetGeometricError` has been removed.
- Removed `TileContentLoadResult::rasterOverlayProjections`. This field is now found in the `overlayDetails`.
- Removed `obtainGlobeRectangle` from `TileUtilities.h`. Use `estimateGlobeRectangle` in `BoundingVolume.h` instead.
- cesium-native now uses the following options with the `glm` library:
  - `GLM_FORCE_XYZW_ONLY`
  - `GLM_FORCE_EXPLICIT_CTOR`
  - `GLM_FORCE_SIZE_T_LENGTH`

##### Additions :tada:

- Added support for the [3DTILES_implicit_tiling](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_implicit_tiling) extension.
- Added support for the [3DTILES_bounding_volume_S2](https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_bounding_volume_S2) extension.
- Added support for raster overlays, including clipping polygons, on any 3D Tiles tileset.
- Added support for external glTF buffers and images.
- Raster overlay level-of detail is now selected using "target screen pixels" rather than the hard-to-interpret geometric error value.
- A `RasterOverlay` can now be configured with a `maximumScreenSpaceError` independent of the screen-space error used for the geometry.
- `RasterOverlay::loadTileProvider` now returns a `SharedFuture`, making it easy to attach a continuation to run when the load completes.
- Added `GltfContent::applyRtcCenter` and `applyGltfUpAxisTransform`.
- Clipping polygon edges now remain sharp even when zooming in past the available geometry detail.
- Added `DebugColorizeTilesRasterOverlay`.
- Added `BoundingRegionBuilder` to `CesiumGeospatial`.
- Added `GlobeRectangle::EMPTY` static field and `GlobeRectangle::isEmpty` method.
- Added the ability to set the coordinates of a `GlobeRectangle` after construction.

##### Fixes :wrench:

- Improved the computation of bounding regions and overlay texture coordinates from geometry, particularly for geometry that crosses the anti-meridian or touches the poles.
- Fixed a bug that would result in incorrect geometry when upsampling a glTF with a position accessor pointing to a bufferView that did not start at the beginning of its buffer.
- Fixed a problem that could cause incorrect distance computation for a degenerate bounding region that is a single point with a min/max height.
- Improved the numerical stability of `GlobeRectangle::computeCenter` and `GlobeRectangle::contains`.
- Error messages are no longer printed to the Output Log when an upsampled tile happens to have a primitive with no vertices.
- Fixed a bug that could cause memory corruption when a decoded Draco mesh was larger than indicated by the corresponding glTF accessor.
- Fixed a bug that could cause the wrong triangle indices to be used for a Draco-encoded glTF.

### v0.9.0 - 2021-11-01

##### Breaking Changes :mega:

- Changed the following properties in CesiumGltf:
  - `BufferView::target` now defaults to `std::nullopt` instead of `Target::ARRAY_BUFFER`.
  - `ClassProperty::type` now defaults to `Type::INT8` instead of empty string.
  - `ClassProperty::componentType` is now an optional string instead of a `JsonValue`.
  - `FeatureTexture::classProperty` is no longer optional, consistent with changes to the extension spec.
  - `Image::mimeType` now defaults to empty string instead of `MimeType::image_jpeg`.
  - `Sampler::magFilter` and `Sampler::minFilter` now default to `std::nullopt` instead of `MagFilter::NEAREST`.
- The version of `ExtensibleObject` in the `CesiumGltf` library and namespace has been removed. Use the one in the `CesiumUtility` library and namespace instead.
- Renamed the following glTF extension classes:
  - `KHR_draco_mesh_compression` -> `ExtensionKhrDracoMeshCompression`.
  - `MeshPrimitiveEXT_feature_metadata` -> `ExtensionMeshPrimitiveExtFeatureMetadata`
  - `ModelEXT_feature_metadata` -> `ExtensionModelExtFeatureMetadata`
- `CesiumGltf::ReaderContext` has been removed. It has been replaced with either `CesiumJsonReader::ExtensionReaderContext` or `GltfReader`.

##### Additions :tada:

- Added new `Cesium3DTiles` and `Cesium3DTilesReader` libraries. They are useful for reading and working with 3D Tiles tilesets.

##### Fixes :wrench:

- Fixed a bug that could cause crashes or incorrect behavior when using raster overlays.
- Fixed a bug that caused 3D Tiles content to fail to load when the status code was zero. This code is used by libcurl for successful read of `file://` URLs, so the bug prevented loading from such URLs in some environments.
- Errors and warnings that occur while loading glTF textures are now include in the model load errors and warnings.
- Fixes how `generate-classes` deals with reserved C++ keywords. Property names that are C++ keywords should be appended with "Property" as was already done,
but when parsing JSONs the original property name string should be used.

### v0.8.0 - 2021-10-01

##### Breaking Changes :mega:

- glTF enums are now represented in CesiumGltf as their underlying type (int32 or string) rather than as an enum class.
- Tile content loaders now return a `Future`, which allows them to be asynchronous and make further network requests.

##### Fixes :wrench:

- Fixed a bug that caused the `RTC_CENTER` semantic in a B3DM feature table to be ignored if any of the values happened to be integers rather than floating-point numbers. This caused these tiles to render in the wrong location.

### v0.7.2 - 2021-09-14

##### Fixes :wrench:

- Fixed a bug where the "forbidHoles" option was not working with raster overlays and external tilesets.

### v0.7.1 - 2021-09-14

##### Fixes :wrench:

- Fixed a bug introduced in v0.7.0 where credits from a `QuadtreeRasterOverlayTileProvider` were not collected and reported.
- Fixed a bug where disabling frustum culling caused external tilesets to not load.

### v0.7.0 - 2021-09-01

##### Breaking Changes :mega:

- Renamed the `Cesium3DTiles` namespace and library to `Cesium3DTilesSelection`.
- Deleted `Cesium3DTilesSelection::Gltf` and moved functionality into `CesiumGltf::Model`.
- Renamed `Rectangle::intersect` and `GlobeRectangle::intersect` to `computeIntersection`.
- `RasterOverlay` and derived classes now require a `name` parameter to their constructors.
- Changed the type of texture coordinate IDs used in the raster overlay system from `uint32_t` to `int32_t`.
- `RasterOverlayTileProvider` is no longer quadtree-oriented. Instead, it requires derived classes to provide an image for a particular requested rectangle and geometric error. Classes that previously derived from `RasterOverlayTileProvider` should now derive from `QuadtreeRasterOverlayTileProvider` and implement `loadQuadtreeTileImage` instead of `loadTileImage`.
- Removed `TilesetOptions::enableWaterMask`, which didn't have any effect anyway. `TilesetContentOptions::enableWaterMask` still exists and works.

##### Additions :tada:

- Added `Future<T>::isReady`.
- Added `Future<T>::share`, which returns a `SharedFuture<T>` and allows multiple continuations to be attached.
- Added an option in `TilesetOptions::ContentOptions` to generate smooth normals when the original glTFs were missing normals.
- Added `ImageManipulation` class to `CesiumGltfReader`.
- Added `Math::roundUp` and `Math::roundDown`.
- Added `Rectangle::computeUnion`.

##### Fixes :wrench:

- Fixed a bug that caused CesiumGltfWriter to write a material's normal texture info into a property named `normalTextureInfo` rather than `normalTexture`.
- Fixed a bug in `TileMapServiceRasterOverlay` that caused it to show only the lowest resolution tiles if missing a `tilemapresource.xml` file.

### v0.6.0 - 2021-08-02

##### Breaking Changes :mega:

- `Future<T>::wait` now returns the resolved value and throws if the Future rejected, rather than returning a `std::variant` and slicing the exception to `std::exception`.
- `Tileset::updateView` and `Tileset::updateViewOffline` now take `std::vector<ViewState>` instead of a single `ViewState`.

##### Additions :tada:

- Added support for the `EXT_feature_metadata` glTF extension.
- Added automatic conversion of the B3DM batch table to the `EXT_feature_metadata` extension.
- Added `CESIUM_COVERAGE_ENABLED` option to the build system.
- Added `AsyncSystem::dispatchOneMainThreadTask` to dispatch a single task, rather than all the tasks that are waiting.
- Added `AsyncSystem::createPromise` to create a Promise directly, rather than via a callback as in `AsyncSystem::createFuture`.
- Added `AsyncSystem::catchImmediately` to catch a Future rejection immediately in any thread.
- Added `AsyncSystem::all` to create a Future that resolves when a list of Futures resolve.
- Added support for multiple frustums in the `Tileset` selection algorithm.

##### Fixes :wrench:

- Fixed a bug that prevented `.then` functions from being used on a `Future<void>` when CESIUM_TRACING_ENABLED was ON.

### v0.5.0 - 2021-07-01

##### Breaking Changes :mega:

- `TilesetExternals` now has an `AsyncSystem` instead of a shared pointer to an `ITaskProcessor`.

##### Additions :tada:

- Added a performance tracing framework via `CESIUM_TRACE_*` macros.
- Added `Future<T>::thenImmediately`.
- Added `AsyncSystem::createThreadPool` and `Future<T>::thenInThreadPool`.
- `Future<T>::thenInWorkerThread` and `Future<T>::thenInMainThread` now arrange for their continuations to be executed immediately when the Future is resolved, if the Future is resolved in the correct thread.
- Moved all request cache database access to a dedicated thread, in order to free up worker threads for parallelizable work.

### v0.4.0 - 2021-06-01

##### Additions :tada:

- Added `Cesium3DTiles::TileIdUtilities` with a `createTileIdString` function to create logging/debugging strings for `TileID` objects.
- Accessing the same Bing Maps layer multiple times in a single application run now reuses the same Bing Maps session instead of starting a new one each time.
- Added a configure-time build option, `PRIVATE_CESIUM_SQLITE`, to rename all `sqlite3*` symbols to `cesium_sqlite3*`.

##### Fixes :wrench:

- Matched draco's decoded indices to gltf primitive if indices attribute does not match with the decompressed indices.
- `createAccessorView` now creates an (invalid) `AccessorView` with a standard numeric type on error, rather than creating `AccessorView<nullptr_t>`. This makes it easier to use a simple lambda as the callback.
- Disabled `HTTPLIB_USE_ZLIB_IF_AVAILABLE` and `HTTPLIB_USE_OPENSSL_IF_AVAILABLE` because these libraries are not required for our use for cpp-httplib and they cause problems on some systems.

### v0.3.1 - 2021-05-13

##### Fixes :wrench:

- Fixed a memory leak when loading textures from a glTF model.
- Fixed a use-after-free bug that could cause a crash when destroying a `RasterOverlay`.

### v0.3.0 - 2021-05-03

##### Breaking Changes :mega:

- Converted `magic_enum` / `CodeCoverage.cmake` dependencies to external submodules.
- Replaced `CesiumGltf::WriteFlags` bitmask with `CesiumGltf::WriteModelOptions` struct.
  `CesiumGltf::writeModelAsEmbeddedBytes` and `CesiumGltf::writeModelAndExternalfiles`
  now use this struct for configuration.
- Removed all exceptions in `WriterException.h`, warnings / errors are now reported in
  `WriteModelResult`, which is returned from `CesiumGltf::writeModelAsEmbeddedBytes` and
  `CesiumGltf::writeModelAndExternalFiles` instead.

##### Additions :tada:

- Added support for loading the water mask from quantized-mesh terrain tiles.

##### Fixes :wrench:

- Let a tile be renderable if all its raster overlays are ready, even if some are still loading.

### v0.2.0 - 2021-04-19

##### Breaking Changes :mega:

- Moved `JsonValue` from the `CesiumGltf` library to the `CesiumUtility` library and changes some of its methods.
- Renamed `CesiumGltf::Reader` to `CesiumGltf::GltfReader`.
- Made the `readModel` and `readImage` methods on `GltfReader` instance methods instead of static methods.

##### Additions :tada:

- Added `CesiumGltfWriter` library.
- Added `CesiumJsonReader` library.
- Added diagnostic details to error messages for invalid glTF inputs.
- Added diagnostic details to error messages for failed OAuth2 authorization with `CesiumIonClient::Connection`.
- Added an `Axis` enum and `AxisTransforms` class for coordinate system transforms
- Added support for the legacy `gltfUpVector` string property in the `asset` part of tilesets. The up vector is read and passed as an `Axis` in the `extras["gltfUpVector"]` property, so that receivers may rotate the glTF model's up-vector to match the Z-up convention of 3D Tiles.
- Unknown glTF extensions are now deserialized as a `JsonValue`. Previously, they were ignored.
- Added the ability to register glTF extensions for deserialization using `GltReader::registerExtension`.
- Added `GltfReader::setExtensionState`, which can be used to request that an extension not be deserialized or that it be deserialized as a `JsonValue` even though a statically-typed class is available for the extension.

##### Fixes :wrench:

- Gave glTFs created from quantized-mesh terrain tiles a more sensible material with a `metallicFactor` of 0.0 and a `roughnessFactor` of 1.0. Previously the default glTF material was used, which has a `metallicFactor` of 1.0, leading to an undesirable appearance.
- Reported zero-length images as non-errors as `BingMapsRasterOverlay` purposely requests that the Bing servers return a zero-length image for non-existent tiles.
- 3D Tiles geometric error is now scaled by the tile's transform.
- Fixed a bug that that caused a 3D Tiles tile to fail to refine when any of its children had an unsupported type of content.

### v0.1.0 - 2021-03-30

- Initial release.
