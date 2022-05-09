# Change Log

### v0.15.1 - 2022-05-05

##### Fixes :wrench:

- Fixed a bug that could cause tiles in external tilesets to fail to load.
- Fixed a bug where upsampled quadtree tiles could have siblings with mismatching projections.

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
