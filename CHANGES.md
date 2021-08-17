# Change Log

### Next Release - ?

##### Additions :tada:

- Added `Future<T>::share`, which returns a `SharedFuture<T>` and allows multiple continuations to be attached.

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
