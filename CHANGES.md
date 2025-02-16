# Change Log

### ? - ?

##### Breaking Changes :mega:

- Removed `TilesetOptions::maximumSimultaneousSubtreeLoads` because it was unused.

##### Additions :tada:

- Added `convertPropertyComponentTypeToAccessorComponentType` to `PropertyType`.
- `LayerJsonTerrainLoader` now includes the query parameters from the base URL in the requests for each `.terrain` file loaded.
- Added support for `3DTILES_ellipsoid` in `Cesium3DTiles`, `Cesium3DTilesReader`, and `Cesium3DTilesWriter`.
- Added support for `3DTILES_content_voxels` in `Cesium3DTiles`, `Cesium3DTilesReader`, and `Cesium3DTilesWriter`.
- Added generated classes for `EXT_primitive_voxels` and its dependencies in `CesiumGltf`, `CesiumGltfReader`, and `CesiumGltfWriter`.

##### Fixes :wrench:

- Fixed parsing URIs that have a scheme followed by `:` instead of `://`.
- Fixed decoding of KHR_mesh_quantization normalized values.

### v0.44.3 - 2025-02-12

##### Fixes :wrench:

- Fixed another bug in `GltfUtilities::parseGltfCopyright` that could cause it to crash or produce incorrect results.

### v0.44.2 - 2025-02-10

##### Fixes :wrench:

- Fixed a bug in `GltfUtilities::parseGltfCopyright` that could cause a crash when the copyright ends with a semicolon.

### v0.44.1 - 2025-02-03

##### Fixes :wrench:

- Fixed a bug in `CesiumIonClient::Connection` that caused the `authorize` method to use an incorrect URL.

### v0.44.0 - 2025-02-03

##### Breaking Changes :mega:

- Removed `Math::rotation`. Use `glm::rotation` from `<glm/gtx/quaternion.hpp>` instead.
- Removed `Math::perpVector`. Use `glm::perp` from `<glm/gtx/perpendicular.hpp>` instead.
- Using Cesium Native in non-cmake projects now requires manually defining `GLM_ENABLE_EXPERIMENTAL`.
- cesium-native no longer uses the `GLM_FORCE_SIZE_T_LENGTH` option with the `glm` library
- `CullingVolume` has been moved from the `Cesium3DTilesSelection` namespace to the `CesiumGeometry` namespace.

##### Additions :tada:

- Added `forEachTile`, `forEachContent`, `addExtensionUsed`, `addExtensionRequired`, `removeExtensionUsed`, `removeExtensionRequired`, `isExtensionUsed`, and `isExtensionRequired` to `Cesium3DTiles::Tileset`.
- Added conversion of I3dm batch table metadata to `EXT_structural_metadata` and `EXT_instance_features` extensions.
- Added `CesiumIonClient::Connection::geocode` method for making geocoding queries against the Cesium ion geocoder API.
- Added `UrlTemplateRasterOverlay` for requesting raster tiles from services using a templated URL.
- `upsampleGltfForRasterOverlays` is now compatible with meshes using TRIANGLE_STRIP, TRIANGLE_FAN, or non-indexed TRIANGLES primitives.
- Added `requestHeaders` field to `TilesetOptions` to allow per-tileset request headers to be specified.

##### Fixes :wrench:

- Fixed a crash in `GltfWriter` that would happen when the `EXT_structural_metadata` `schema` property was null.
- Fixed a bug in `SharedAssetDepot` that could cause assertion failures in debug builds, and could rarely cause premature deletion of shared assets even in release builds.
- Fixed a bug that could cause `Tileset::sampleHeightMostDetailed` to return a height that is not the highest one when the sampled tileset contained multiple heights at the given location.
- `LayerJsonTerrainLoader` will now log errors and warnings when failing to load a `.terrain` file referenced in the layer.json, instead of silently ignoring them.
- URIs containing unicode characters are now supported.
- Fixed a crash in `CullingVolume` when the camera was very far away from the globe.
- Fixed a bug that prevented the `culture` parameter of the `BingMapsRasterOverlay` from having an effect.

### v0.43.0 - 2025-01-02

##### Breaking Changes :mega:

- Removed unused types `JsonValueMissingKey` and `JsonValueNotRealValue` from `CesiumUtility`.

##### Additions :tada:

- Added `offset` getter to `AccessorView`.
- Added `stride`, `offset`, and `data` getters to `AccessorWriter`.
- Added `value_type` typedef to `AccessorWriter`.
- Added `InstanceAttributeSemantics` to `CesiumGltf`.
- Added `VertexAttributeSemantics::FEATURE_ID_n`.
- Added a `const` version of `Tileset::forEachLoadedTile`.
- Added `DebugTileStateDatabase`, which provides tools for debugging the tile selection algorithm using SQLite.
- Added `CesiumAsync::SqliteHelper`, containing functions for working with SQLite.
- Updates generated classes for `EXT_structural_metadata`. See https://github.com/CesiumGS/glTF/pull/71.

##### Fixes :wrench:

- Fixed a bug in `thenPassThrough` that caused a compiler error when given a value by r-value refrence.
- Fixed a raster overlay bug that could cause unnecessary upsampling with failed or missing overlay tiles.
- Fixed a bug in  `SubtreeFileReader::loadBinary` that prevented valid subtrees from loading if they did not contain binary data.
- Fixed a bug in the `Tileset` selection algorithm that could cause detail to disappear during load in some cases.
- Improved the "kicking" mechanism in the tileset selection algorithm. The new criteria allows holes in a `Tileset`, when they do occur, to be filled with loaded tiles more incrementally.
- Fixed a bug in `SharedAssetDepot` that could lead to crashes and other undefined behavior when an asset in the depot outlived the depot itself.
- Fixed a bug that could cause some rotations in an Instanced 3D Model (.i3dm) to be represented incorrectly.

### v0.42.0 - 2024-12-02

##### Breaking Changes :mega:

- Cesium Native now requires C++20 and uses vcpkg `2024.11.16`.
- Switched from `gsl::span` to `std::span` throughout the library and API. The GSL library has been removed.
- The `BingMapsRasterOverlay` constructor no longer takes an `ellipsoid` parameter. Instead, it uses the ellipsoid specified in `RasterOverlayOptions`.
- The `ellipsoid` field in `RasterOverlayOptions` is no longer a `std::optional`. Instead, it defaults to WGS84 directly.
- Removed the `ellipsoid` field from `TileMapServiceRasterOverlayOptions`, `WebMapServiceRasterOverlayOptions`, and `WebMapTileServiceRasterOverlayOptions`. These overlays now use the ellipsoid in `RasterOverlayOptions` instead.
- The `schema` property of `ExtensionModelExtStructuralMetadata` is now an `IntrusivePointer` instead of a `std::optional`.

##### Additions :tada:

- Added support for `EXT_accessor_additional_types` in `AccessorView`.
- Added `EllipsoidTilesetLoader` that will generate a tileset by tessellating the surface of an ellipsoid, producing a simple globe tileset without any terrain features.
- External schemas referenced by the `schemaUri` property in the `EXT_structural_metadata` glTF extension are now loaded automatically. Two models that reference the same external schema will share a single copy of it.
- Added `getHeightSampler` method to `TilesetContentLoader`, allowing loaders to optionally provide a custom, more efficient means of querying heights using the `ITilesetHeightSampler` interface.
- Added equality operator for `JsonValue`.
- `TileLoadResult` now includes a `pAssetAccessor` that was used to retrieve the tile content and that should be used to retrieve any additional resources associated with the tile, such as external images.

##### Fixes :wrench:

- Updated the CMake install process to install the vcpkg-built Debug binaries in Debug builds. Previously the Release binaries were installed instead.
- Fixed a crash that would occur for raster overlays attempting to dereference a null `CreditSystem`.
- Fixed a bug where an empty `extensions` object would get written if an `ExtensibleObject` only had unregistered extensions.
- Tightened the tolerance of `IntersectionTests::rayTriangleParametric`, allowing it to find intersections with smaller triangles.
- Fixed a bug that could cause `GltfUtilities::intersectRayGltfModel` to crash when the model contains a primitive whose position accessor does not have min/max values.
- `IonRasterOverlay` now passes its `RasterOverlayOptions` to the `BingMapsRasterOverlay` or `TileMapServiceRasterOverlay` that it creates internally.
- Fixed a bug in `CachingAssetAccessor` that caused it to return cached request headers on a cache hit, rather than the headers included in the new request.
- External resources (such as images) referenced from 3D Tiles content will no longer fail if a Cesium ion token refresh is necessary.
- The Cesium ion token will now only be refreshed once when it expires. Previously, multiple refresh requests could be initiated at about the same time.
- Fixed a bug in `SharedAssetDepot` that could lead to a crash with assets that fail to load.
- Fixed a bug in `AccessorView` that could cause it to report the view as valid even when its `BufferView` had a negative `byteStride`.

### v0.41.0 - 2024-11-01

##### Breaking Changes :mega:

- Renamed `CesiumUtility/Gunzip.h` to `CesiumUtility/Gzip.h`.
- Renamed `ImageCesium` to `ImageAsset`.
- The `cesium` field in `CesiumGltf::Image` is now named `pAsset` and is an `IntrusivePointer` to an `ImageAsset`.
- The `image` field in `LoadedRasterOverlayImage` is now named `pImage` and is an `IntrusivePointer` to an `ImageAsset`.
- Deprecated the `readImage` and `generateMipMaps` methods on `GltfReader`. These methods are now found on `ImageDecoder`.

##### Additions :tada:

- Added `CesiumUtility::gzip`.
- Added `CesiumGeometry::Transforms::getUpAxisTransform` to get the transform that converts from one up axis to another.
- Added `TilesetSharedAssetSystem` to `Cesium3DTilesSelection` and `GltfSharedAssetSystem` to `CesiumGltfReader`.
- Added `SharedAsset` to `CesiumUtility` to serve as the base class for assets such as `ImageAsset`.
- Added `SharedAssetDepot` to `CesiumAsync` for managing assets, such as images, that can be shared among multiple models or other objects.
- Added `NetworkAssetDescriptor` and `NetworkImageAssetDescriptor`.
- `ImageAsset` (formerly `ImageCesium`) is now an `ExtensibleObject`.
- Added `VertexAttributeSemantics` to `CesiumGltf`.
- Added `ImageDecoder` to `CesiumGltfReader`.
- Added `DoublyLinkedListAdvanced` to `CesiumUtility`. It is equivalent to `DoublyLinkedList` except it allows the next and previous pointers to be in a base class of the node class.
- Added `contains` method to `DoublyLinkedList` (and `DoublyLinkedListAdvanced`).
- Added static `error` and `warning` methods to `ErrorList`, making it easy to create an instance with a single error or warning.
- `ExtensibleObject::addExtension` now takes arguments that are passed through to the extension's constructor.
- Added `Hash` to `CesiumUtility`.
- Added `emplace` and `reset` methods to `IntrusivePointer`.
- Added `Result<T>` and `ResultPointer<T>` classes to represent the result of an operation that might complete with warnings and errors.

##### Fixes :wrench:

- Fixed missing ellipsoid parameters that would lead to incorrect results when using non-WGS84 ellipsoids.
- Fixed a bug in `AsyncSystem::all` where the resolved values of individual futures were copied instead of moved into the output array.
- Improved the hash function for `QuadtreeTileID`.

### v0.40.1 - 2024-10-01

##### Fixes :wrench:

- Fixed a regression in v0.40.0 that could cause tilesets with raster overlays to fail to load in some cases.

### v0.40.0 - 2024-10-01

##### Breaking Changes :mega:

- Renamed `shouldContentContinueUpdating` to `getMightHaveLatentChildren` and `setContentShouldContinueUpdating` to `setMightHaveLatentChildren` on the `Tile` class.
- `LoadedRasterOverlayImage` now has a single `errorList` property instead of separate `errors` and `warnings` properties.

##### Additions :tada:

- Added `sampleHeightMostDetailed` method to `Tileset`.
- `AxisAlignedBox` now has `constexpr` constructors.

##### Fixes :wrench:

- Fixed a bug that prevented use of `Tileset` with a nullptr `IPrepareRendererResources`.
- Fixed a bug in `IntersectionTests::rayOBBParametric` that could cause incorrect results for some oriented bounding boxes.
- `GltfUtilities::intersectRayGltfModel` now reports a warning when given a model it can't compute the intersection with because it uses required extensions that are not supported.
- Errors while loading raster overlays are now logged. Previously, they were silently ignored in many cases.
- A raster overlay image failing to load will no longer completely prevent the geometry tile to which it is attached from rendering. Instead, once the raster overlay fails, the geometry tile will be shown without the raster overlay.
- Fixed a bug in the various `catchImmediately` and `catchInMainThread` functions in `CesiumAsync` that prevented use of a mutable lambda.

### v0.39.0 - 2024-09-02

##### Breaking Changes :mega:

- Setting the CMake variable `PRIVATE_CESIUM_SQLITE` will no longer automatically rename all of the SQLite symbols. It must also be paired with a vcpkg overlay port that renames the symbols in SQLite itself.
- `PropertyArrayView` is now exclusively a view, with no ability to own the data it is viewing. The new `PropertyArrayCopy` can be used when an owning view is required.

##### Additions :tada:

- Added `CesiumGltfWriter::SchemaWriter` for serializing schemas in [EXT_structural_metadata](https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_structural_metadata).
- Added `resolveExternalImages` flag to `GltfReaderOptions`, which is true by default.
- Added `removeExtensionUsed` and `removeExtensionRequired` methods to `CesiumGltf::Model`.
- Added `getFeatureIdAccessorView` overload for retrieving feature IDs from `EXT_instance_features`.
- Added `CesiumGeospatial::EarthGravitationalModel1996Grid` class to allow transforming heights on a WGS84 ellipsoid into heights above mean sea level using the EGM96 model.

##### Fixes :wrench:

- Fixed a bug in `WebMapTileServiceRasterOverlay` that caused it to compute the `TileRow` incorrectly when used with a tiling scheme with multiple tiles in the Y direction at the root.
- `KHR_texture_transform` is now removed from `extensionsUsed` and `extensionsRequired` after it is applied by `GltfReader`.
- Fixed a bug in the i3dm loader that caused glTF with multiple nodes to not be instanced correctly.

### v0.38.0 - 2024-08-01

##### Breaking Changes :mega:

- `AccessorWriter` constructor now takes `std::byte*` instead of `uint8_t*`.

##### Additions :tada:

- Added `rayTriangle` intersection function that returns the intersection point between a ray and a triangle.
- Added `intersectRayGltfModel` intersection function that returns the first intersection point between a ray and a glTF model.
- Added `convertAccessorComponentTypeToPropertyComponentType`, which converts integer glTF accessor component types to their best-fitting `PropertyComponentType`.

##### Fixes :wrench:

- Fixed a bug that prevented raster overlays from being correctly applied when a non-standard "glTF up axis" is in use.

### v0.37.0 - 2024-07-01

##### Additions :tada:

- Added full support for custom ellipsoids by setting `TilesetOptions::ellipsoid` when creating a tileset.
  - Many methods have been updated with an additional ellipsoid parameter to support this. The WGS84 ellipsoid is used as a default parameter here to ensure API compatibility.
  - `CESIUM_DISABLE_DEFAULT_ELLIPSOID` can be defined to disable the WGS84 default parameter, exposing through errors the places in your code that are still assuming a WGS84 ellipsoid.
- Added `removeUnusedMeshes` and `removeUnusedMaterials` to `GltfUtilities`.
- Added `rayEllipsoid` static method to `CesiumGeometry::IntersectionTests`.
- Added equality operator for `Cartographic`.
- Added `CESIUM_MSVC_STATIC_RUNTIME_ENABLED` option to the CMake scripts. It is OFF by default, and when enabled, configures any MS visual studio projects for the "Multi-threaded" (/MT) runtime library rather than "Multi-threaded DLL" (/MD)

##### Fixes :wrench:

- Fixed several problems with the loader for the 3D Tiles Instanced 3D Mesh (i3dm) format:
  - When an instance transform cannot be decomposed into position, rotation, and scale, a warning will now be logged and an identity transformation will be used. Previously, an undefined transformation would be used.
  - The `gltfUpAxis` property is now accounted for, if present.
  - Paths to images in i3dm content are now resolved correctly.
  - Extraneous spaces at the end of an external glTF URI are now ignored. These are sometimes added as padding in order to meet alignment requirements.
- Removed an overly-eager degenerate triangle test in the 2D version of `IntersectionTests::pointInTriangle` that could discard intersections in small - but valid - triangles.
- Fixed a bug while upsampling tiles for raster overlays that could cause them to have an incorrect bounding box, which in some cases would lead to the raster overlay being missing entirely from the upsampled tile.

### v0.36.0 - 2024-06-03

##### Breaking Changes :mega:

- `FeatureId::propertyTable` is now `int32_t` instead of `std::optional<int64_t>`
- `ExtensionMeshPrimitiveExtStructuralMetadata::propertyTextures` and `ExtensionMeshPrimitiveExtStructuralMetadata::propertyAttributes` are now vectors of `int32_t` instead of `int64_t`.

##### Additions :tada:

- Added support for I3DM 3D Tile content files.
- Added `forEachNodeInScene` to `CesiumGltf::Model`.
- Added `removeUnusedBuffers` to `GltfUtilities`.
- Added the following new methods to the `Uri` class: `unescape`, `unixPathToUriPath`, `windowsPathToUriPath`, `nativePathToUriPath`, `uriPathToUnixPath`, `uriPathToWindowsPath`, and `uriPathToNativePath`.
- Added `LayerWriter` to the `CesiumQuantizedMeshTerrain` library and namespace.
- Drastically improved the performance of `GltfUtilities::collapseToSingleBuffer` for glTFs with many buffers and bufferViews.

##### Fixes :wrench:

- Added support for the following glTF extensions to `Model::merge`. Previously these extensions could end up broken after merging.
  - `KHR_texture_basisu`
  - `EXT_texture_webp`
  - `EXT_mesh_gpu_instancing`
  - `EXT_meshopt_compression`
  - `CESIUM_primitive_outline`
  - `CESIUM_tile_edges`
- Fixed a bug in `GltfUtilities::compactBuffer` where it would not preserve the alignment of the bufferViews.
- The `collapseToSingleBuffer` and `moveBufferContent` functions in `GltfUtilities` now align to an 8-byte boundary rather than a 4-byte boundary, because bufferViews associated with some glTF extensions require this larger alignment.
- `GltfUtilities::collapseToSingleBuffer` now works correctly even if some of the buffers in the model have a `uri` property and the data at that URI has not yet been loaded. Such buffers are left unmodified.
- `GltfUtilities::collapseToSingleBuffer` now works correctly with bufferViews that have the `EXT_meshopt_compression` extension.
- `GltfUtilities::compactBuffer` now accounts for bufferViews with the `EXT_meshopt_compression` when determining unused buffer ranges.
- When `GltfReader` decodes buffers with data URLs, and the size of the data in the URL does not match the buffer's `byteLength`, the `byteLength` is now updated and a warning is raised. Previously, the mismatch was ignored and would cause problems later when trying to use these buffers.
- `EXT_meshopt_compression` and `KHR_mesh_quantization` are now removed from `extensionsUsed` and `extensionsRequired` after they are decoded by `GltfReader`.
- The glTF accessor for the texture coordinates created by `RasterOverlayUtilities::createRasterOverlayTextureCoordinates` now has min/max values that accurately reflect the range of values. Previously, the minimum was always set to 0.0 and the maximum to 1.0.
- Fixed a bug in the `waitInMainThread` method on `Future` and `SharedFuture` that could cause it to never return if the waited-for future rejected.
- Moved the small amount of Abseil code embedded into the s2geometry library from the `absl` namespace to the `cesium_s2geometry_absl` namespace, in order to avoid linker errors when linking against both cesium-native and the full Abseil library.
- Fixed a crash in `ExtensionWriterContext` when attempting to write statically-typed extensions that aren't registered. Now a warning is reported.

### v0.35.0 - 2024-05-01

##### Breaking Changes :mega:

- Moved `upsampleGltfForRasterOverlays` into `RasterOverlayUtilities`. Previously it was a global function. Also added two new parameters to it, prior to the existing `textureCoordinateIndex` parameter.
- Moved `QuantizedMeshLoader` from `Cesium3DTilesContent` to `CesiumQuantizedMeshTerrain`. If experiencing related linker errors, add `CesiumQuantizedMeshTerrain` to the libraries you link against.
- `Connection::authorize` now requires an `ApplicationData` parameter, which represents the `appData` retrieved from a Cesium ion server.

##### Additions :tada:

- Added a new `CesiumQuantizedMeshTerrain` library and namespace, containing classes for working with terrain in the `quantized-mesh-1.0` format and its `layer.json` file.
- Added `getComponentCountFromPropertyType` to `PropertyType`.
- Added `removeExtension` to `ExtensibleObject`.
- Added `IndexFromAccessor` to retrieve the index supplied by `IndexAccessorType`.
- Added `NormalAccessorType`, which is a type definition for a normal accessor. It can be constructed using `getNormalAccessorView`.
- Added `Uri::getPath` and `Uri::setPath`.
- Added `TileTransform::setTransform`.
- Added `GlobeRectangle::splitAtAntiMeridian`.
- Added `BoundingRegionBuilder::toGlobeRectangle`.
- Added `GlobeRectangle::equals` and `GlobeRectangle::equalsEpsilon`.
- `upsampleGltfForRasterOverlays` now accepts two new parameters, `hasInvertedVCoordinate` and `textureCoordinateAttributeBaseName`.
- `upsampleGltfForRasterOverlays` now copies images from the parent glTF into the output model.
- Added `waitInMainThread` method to `Future` and `SharedFuture`.
- Added `forEachRootNodeInScene`, `addExtensionUsed`, `addExtensionRequired`, `isExtensionUsed`, and `isExtensionRequired` methods to `CesiumGltf::Model`.
- Added `getNodeTransform`, `setNodeTransform`, `removeUnusedTextures`, `removeUnusedSamplers`, `removeUnusedImages`, `removeUnusedAccessors`, `removeUnusedBufferViews`, and `compactBuffers` methods to `GltfUtilities`.
- Added `postprocessGltf` method to `GltfReader`.
- `Model::merge` now merges the `EXT_structural_metadata` and `EXT_mesh_features` extensions. It also now returns an `ErrorList`, used to report warnings and errors about the merge process.

##### Fixes :wrench:

- Fixed a bug in `joinToString` when given a collection containing empty strings.
- `QuantizedMeshLoader` now creates spec-compliant glTFs from a quantized-mesh terrain tile. Previously, the generated glTF had small problems that could confuse some clients.
- Fixed a bug in `TileMapServiceRasterOverlay` that caused it to build URLs incorrectly when given a URL with query parameters.
- glTFs converted from a legacy batch table to a `EXT_structural_metadata` now:
  - Add the `EXT_structural_metadata` and `EXT_mesh_features` extensions to the glTF's `extensionsUsed` list.
  - Omit property table properties without any values at all. Previously, such property table properties would have a `values` field referring to an invalid bufferView, which is contrary to the extension's specification.
  - Rename the `_BATCHID` attribute to `_FEATURE_ID_0` inside the `KHR_draco_mesh_compression` extension (if present), in addition to the primitive's `attributes`. Previously, meshes still Draco-compressed after the upgrade, by setting `options.decodeDraco=false`, did not have the proper attribute name.
- glTFs converted from 3D Tiles B3DMs with the `RTC_CENTER` property will now have `CESIUM_RTC` added to their `extensionsRequired` and `extensionsUsed` lists.
- glTFs converted from the 3D Tiles PNTS format now:
  - Have their `asset.version` field correctly set to `"2.0"`. Previously the version was not set, which is invalid.
  - Have the `KHR_materials_unlit` extension added to the glTF's `extensionsUsed` list when the point cloud does not have normals.
  - Have a default `scene`.
  - Have the `CESIUM_RTC` extension added to the glTF's `extensionsRequired` and `extensionsUsed` lists when the PNTS uses the `RTC_CENTER` property.
- When glTFs are loaded with `applyTextureTransform` set to true, the accessors and bufferViews created for the newly-generated texture coordinates now have their `byteOffset` set to zero. Previously, they inherited the value from the original `KHR_texture_transform`-dependent objects, which was incorrect.
- `bufferViews` created for indices during Draco decoding no longer have their `byteStride` property set, as this is unnecessary and disallowed by the specification.
- `bufferViews` created for vertex attributes during Draco decoding now have their `target` property correctly set to `BufferView::Target::ARRAY_BUFFER`.
- After a glTF has been Draco-decoded, the `KHR_draco_mesh_compression` extension is now removed from the primitives, as well as from `extensionsUsed` and `extensionsRequired`.
- For glTFs converted from quantized-mesh tiles, accessors created for the position attribute now have their minimum and maximum values set correctly to include the vertices that form the skirt around the edge of the tile.
- Fixed some glTF validation problems with the mode produced by `upsampleGltfForRasterOverlays`.
- `RasterOverlayUtilities::createRasterOverlayTextureCoordinates` no longer fails when the model spans the anti-meridian. However, only the larger part of the model on one side of the anti-meridian will have useful texture coordinates.
- Fixed a bug that caused `GltfWriter` to create an invalid GLB if its total size would be greater than or equal to 4 GiB. Because it is not possible to produce a valid GLB of this size, GltfWriter now reports an error instead.
- `CesiumUtility::Uri::resolve` can now properly parse protocol-relative URIs (such as `//example.com`).
- Fixed a bug where the `GltfReader` was not able to read a model when the BIN chunk of the GLB data was more than 3 bytes larger than the size of the JSON-defined `buffer`.

### v0.34.0 - 2024-04-01

##### Breaking Changes :mega:

- Renamed `IntersectionTests::pointInTriangle2D` to `IntersectionTests::pointInTriangle`.

##### Additions :tada:

- Added `AccessorWriter` constructor that takes an `AccessorView`.
- Added `PositionAccessorType`, which is a type definition for a position accessor. It can be constructed using `getPositionAccessorView`.
- Added overloads of `IntersectionTests::pointInTriangle` that handle 3D points. One overload includes a `barycentricCoordinates` parameter that outputs the barycentric coordinates at that point.
- Added overloads of `ImplicitTilingUtilities::computeBoundingVolume` that take a `Cesium3DTiles::BoundingVolume`.
- Added overloads of `ImplicitTilingUtilities::computeBoundingVolume` that take an `S2CellBoundingVolume` and an `OctreeTileID`. Previously only `QuadtreeTileID` was supported.
- Added `setOrientedBoundingBox`, `setBoundingRegion`, `setBoundingSphere`, and `setS2CellBoundingVolume` functions to `TileBoundingVolumes`.

##### Fixes :wrench:

- Fixed a bug where coordinates returned from `SimplePlanarEllipsoidCurve` were inverted if one of the input points had a negative height.
- Fixed a bug where `Tileset::ComputeLoadProgress` could incorrectly report 100% before all tiles finished their main thread loading.

### v0.33.0 - 2024-03-01

##### Breaking Changes :mega:

- Removed support for `EXT_feature_metadata` in `CesiumGltf`, `CesiumGltfReader`, and `CesiumGltfWriter`. This extension was replaced by `EXT_mesh_features`, `EXT_instance_features`, and `EXT_structural_metadata`.
- Moved `ReferenceCountedNonThreadSafe<T>` to `ReferenceCounted.h`. It is also now a type alias for `ReferenceCounted<T, false>` rather than an actual class.
- Renamed `applyKHRTextureTransform` to `applyKhrTextureTransform`. The corresponding header file was similarly renamed to `CesiumGltf/applyKhrTextureTransform.h`.

##### Additions :tada:

- Added `TextureViewOptions`, which includes the following flags:
  - `applyKhrTextureTransformExtension`: When true, the view will automatically transform texture coordinates before sampling the texture.
  - `makeImageCopy`: When true, the view will make its own CPU copy of the image data.
- Added `TextureView`. It views an arbitrary glTF texture and can be affected by `TextureViewOptions`. `FeatureIdTextureView` and `PropertyTexturePropertyView` now inherit from this class.
- Added `options` parameter to `PropertyTextureView::getPropertyView` and `PropertyTextureView::forEachProperty`, allowing views to be constructed with property-specific options.
- Added `KhrTextureTransform`, a utility class that parses the `KHR_texture_transform` glTF extension and reports whether it is valid. UVs may be transformed on the CPU using `applyTransform`.
- Added `contains` method to `BoundingSphere`.
- Added `GlobeRectangle::MAXIMUM` static field.
- Added `ReferenceCountedThreadSafe` type alias.
- Added `SimplePlanarEllipsoidCurve` class to help with calculating fly-to paths.
- Added `sizeBytes` field to `ImageCesium`, allowing its size to be tracked for caching purposes even after its `pixelData` has been cleared.
- Added `scaleToGeocentricSurface` method to `Ellipsoid`.

##### Fixes :wrench:

- Fixed a bug in `BoundingVolume::estimateGlobeRectangle` where it returned an incorrect rectangle for boxes and spheres that encompass the entire globe.
- Fixed an incorrect computation of wrapped texture coordinates in `applySamplerWrapS` and `applySamplerWrapT`.

### v0.32.0 - 2024-02-01

##### Breaking Changes :mega:

- `IndicesForFaceFromAccessor` now properly supports `TRIANGLE_STRIP` and `TRIANGLE_FAN` modes. This requires the struct to be initialized with the correct primitive mode.

##### Additions :tada:

- Added support for Web Map Tile Service (WMTS) with `WebMapTileServiceRasterOverlay`.
- Added conversions from `std::string` to other metadata types in `MetadataConversions`. This enables the same conversions as `std::string_view`, while allowing runtime engines to use `std::string` for convenience.
- Added `applyTextureTransform` property to `TilesetOptions`, which indicates whether to preemptively apply transforms to texture coordinates for textures with the `KHR_texture_transform` extension.
- Added `loadGltf` method to `GltfReader`, making it easier to do a full, asynchronous load of a glTF.

##### Fixes :wrench:

- Fixed a bug in `FeatureIdTextureView` where it ignored the wrap values specified on the texture's sampler.
- Fixed a bug that could cause binary implicit tiling subtrees with buffers padded to 8-bytes to fail to load.
- Fixed a bug where upgraded batch table properties were not always assigned sentinel values, even when such values were available and required.
- Fixed incorrect behavior in `PropertyTablePropertyView` where `arrayOffsets` were treated as byte offsets, instead of as array indices.

### v0.31.0 - 2023-12-14

##### Additions :tada:

- Add `defaults` method to `CesiumIonClient::Connection`.

##### Fixes :wrench:

- Fixed a crash in `SubtreeAvailability::loadSubtree`.
- Fixed a bug where the `getApiUrl` method of `CesiumIonClient::Connection` would not return the default API URL if the attempt to access `config.json` failed in a more serious way, such as because of an invalid hostname.

### v0.30.0 - 2023-12-01

##### Breaking Changes :mega:

- Moved `ErrorList`, `CreditSystem`, and `Credit` from `Cesium3DTilesSelection` to `CesiumUtility`.
- Moved `GltfUtilities` from `Cesium3DTilesSelection` to `Cesium3DTilesContent`.
- Moved `RasterOverlay`, `RasterOverlayTileProvider`, `RasterOverlayTile`, `QuadtreeRasterOverlayTileProvider`, `RasterOverlayLoadFailure`, `RasterOverlayDetails`, and all of the `RasterOverlay`-derived types to a new `CesiumRasterOverlays` library and namespace.
- Moved `createRasterOverlayTextureCoordinates` method from `GltfUtilities` to a new `RasterOverlayUtilities` class in the `CesiumRasterOverlays` library.
- `GltfUtilities::parseGltfCopyright` now returns the credits as a vector of `std::string_view` instances. Previously it took a `CreditSystem` and created credits directly.
- The `SubtreeAvailability` constructor and `loadSubtree` static method now take an `ImplicitTileSubdivisionScheme` enumeration parameter instead of a `powerOf2` parameter. They also now require a `levelsInSubtree` parameter, which is needed when switching from constant to bitstream availability. Lastly, the constructor now takes a `Subtree` parameter instead of a `std::vector<std::vector<std::byte>>` representing the buffers.
- `SubtreeConstantAvailability`, `SubtreeBufferViewAvailability`, and `AvailabilityView` are now members of `SubtreeAvailability`.
- Moved `ImageManipulation` from `CesiumGltfReader` to `CesiumGltfContent`.
- Added some new parameters to `RasterOverlayUtilities::createRasterOverlayTextureCoordinates` and changed the order of some existing parameters.

##### Additions :tada:

- Added new `Cesium3DTilesContent` library and namespace. It has classes for loading, converting, and manipulating 3D Tiles tile content.
- Added new `CesiumGltfContent` library and namespace. It has classes for manipulating in-memory glTF files.
- Added new `CesiumRasterOverlays` library and namespace. It has classes for working with massive textures draped over glTFs and 3D Tiles.
- Added `MetadataConversions`, which enables metadata values to be converted to different types for better usability in runtime engines.
- Added various `typedef`s to catch all possible types of `AccessorView`s for an attribute, including `FeatureIdAccessorType` for feature ID attribute accessors, `IndexAccessorType` for index accessors, and `TexCoordAccessorType` for texture coordinate attribute accessors.
- Added `getFeatureIdAccessorView`, `getIndexAccessorView`, and `getTexCoordAccessorView` to retrieve the `AccessorView` as a `FeatureIdAccessorType`, `IndexAccessorType`, or `TexCoordAccessorType` respectively.
- Added `StatusFromAccessor` and `CountFromAccessor` visitors to retrieve the accessor status and size respectively. This can be used with `FeatureIdAccessorType`, `IndexAccessorType`, or `TexCoordAccessorType`.
- Added `FeatureIdFromAccessor` to retrieve feature IDs from a `FeatureIdAccessorType`.
- Added `IndicesForFaceFromAccessor` to retrieve the indices of the vertices that make up a face, as supplied by `IndexAccessorType`.
- Added `TexCoordFromAccessor` to retrieve the texture coordinates from a `TexCoordAccessorType`.
- Added `TileBoundingVolumes` class to `Cesium3DTilesContent`, making it easier to create the rich bounding volume types in `CesiumGeometry` and `CesiumGeospatial` from the simple vector representations in `Cesium3DTiles`.
- Added `transform` method to `CesiumGeometry::BoundingSphere`.
- Added `toSphere`, `fromSphere`, and `fromAxisAligned` methods to `CesiumGeometry::OrientedBoundingBox`.
- Added `TileTransform` class to `Cesium3DTilesContent`, making it easier to create a `glm::dmat4` from the `transform` property of a `Cesium3DTiles::Tile`.
- Added `ImplicitTilingUtilities` class to `Cesium3DTilesContent`.
- Added overloads of `isTileAvailable`, `isContentAvailable`, and `isSubtreeAvailable` on the `SubtreeAvailability` class that take the subtree root tile ID and the tile ID of interest, instead of a relative level and Morton index.
- Added `fromSubtree` and `createEmpty` static methods to `SubtreeAvailability`.
- Added new `set` methods to `SubtreeAvailability`, allowing the availability information to be modified.
- Added `SubtreeFileReader` class, used to read `Cesium3DTiles::Subtree` from a binary or JSON subtree file.
- Added `pointInTriangle2D` static method to `CesiumGeometry::IntersectionTests`.
- Added `rectangleIsWithinPolygons` and `rectangleIsOutsidePolygons` static methods to `CartographicPolygon`.
- Raster overlays now use `IPrepareRasterOverlayRendererResources`, which contains only overlay-related methods, instead of `IPrepareRendererResources`, which contains tileset-related methods as well. `IPrepareRendererResources` derives from `IPrepareRasterOverlayRendererResources` so existing code should continue to work without modification.
- Added `collapseToSingleBuffer` and `moveBufferContent` methods to `GltfUtilities`.
- Added `savePng` method to `ImageManipulation`.
- `RasterOverlayTileProvider::loadTile` now returns a future that resolves when the tile is done loading.
- Added `computeDesiredScreenPixels` and `computeTranslationAndScale` methods to `RasterOverlayUtilities`.
- Added `Future<T>::thenPassThrough`, used to easily pass additional values through to the next continuation.

##### Fixes :wrench:

- Fixed a bug in `OrientedBoundingBox::contains` where it didn't account for the bounding box's center.
- Fixed compiler error when calling `PropertyAttributeView::forEachProperty`.
- Fixed crash when loading glTFs with data uri images.
- Fixed WD4996 warnings-as-errors when compiling with Visual Studio 2002 v17.8.

### v0.29.0 - 2023-11-01

##### Breaking Changes :mega:

- Removed `PropertyTablePropertyViewType` and `NormalizedPropertyTablePropertyViewType`, as well as their counterparts for property textures and property attributes. When compiled with Clang, the large `std::variant` definitions would significantly stall compilation.

##### Fixes :wrench:

- Updated the Cesium ion OAuth2 URL from `https://cesium.com/ion/oauth` to `https://ion.cesium.com/oauth`, avoiding a redirect.

### v0.28.1 - 2023-10-02

##### Breaking Changes :mega:

- Cesium Native is now only regularly tested on Visual Studio 2019+, GCC 11.x+, and Clang 12+. Other compilers - including older ones - are likely to work, but are not tested.

##### Additions :tada:

- Added `getClass` to `PropertyTableView`, `PropertyTextureView`, and `PropertyAttributeView`. This can be used to retrieve the metadata `Class` associated with the view.
- Added `PropertyViewStatus::EmptyPropertyWithDefault` to indicate when a property contains no data, but has a valid default value.
- A glTF `bufferView` with a `byteStride` of zero is now treated as if the `byteStride` is not defined at all. Such a glTF technically violates the spec (the minimum value is 4), but the new behavior is sensible enough and consistent with CesiumJS.

##### Fixes :wrench:

- Fixed the handling of omitted metadata properties in `PropertyTableView`, `PropertyTextureView`, and `PropertyAttributeView` instances. Previously, if a property was not `required` and omitted, it would be initialized as invalid with the `ErrorNonexistentProperty` status. Now, it will be treated as valid as long as the property defines a valid `defaultProperty`. A special instance of `PropertyTablePropertyView`, `PropertyTexturePropertyView`, or `PropertyAttributePropertyView` will be constructed to allow the property's default value to be retrieved, either via `defaultValue` or `get`. `getRaw` may not be called on this special instance.

### v0.28.0 - 2023-09-08

##### Breaking Changes :mega:

- Views of the data contained by `EXT_feature_metadata` will no longer supported by Cesium Native. The extension will still be parsed, but it will log a warning.
- Batch tables will be converted to `EXT_structural_metadata` instead of `EXT_feature_metadata`.
- In `CesiumGltf`, all generated classes related to `EXT_feature_metadata` are now prefixed with `ExtensionExtFeatureMetadata`. For example, `ClassProperty` has become `ExtensionExtFeatureMetadataClassProperty`. This also extends to the glTF reader and writer.
- In `CesiumGltf`, all generated classes related to `EXT_structural_metadata` have had their `ExtensionExtStructuralMetadata` prefix removed. For example, `ExtensionExtStructuralMetadataClassProperty` has become `ClassProperty`. This also extends to the glTF reader and writer.
- In `CesiumGltf`, `ExtensionExtMeshFeaturesFeatureId` and `ExtensionExtMeshFeaturesFeatureIdTexture` have been renamed to `FeatureId` and `FeatureIdTexture` respectively.
- Replaced `FeatureIDTextureView` with `FeatureIdTextureView`, which views a `FeatureIdTexture` in `EXT_mesh_features`. Feature ID textures from `EXT_feature_metadata` are no longer supported.
- Replaced `MetadataFeatureTableView` with `PropertyTableView`, which views a `PropertyTable` in `EXT_structural_metadata`.
- Replaced `MetadataPropertyView` with `PropertyTablePropertyView`, which is a view of a `PropertyTableProperty` in `EXT_structural_metadata`. This takes two template parameters: a typename `T` , and a `bool` indicating whether or not the values are normalized.
- Replaced `MetadataPropertyViewStatus` with `PropertyTablePropertyViewStatus`. `PropertyTablePropertyViewStatus` is a class that inherits from `PropertyViewStatus`, defining additional error codes in the form of `static const` values.
- Replaced `FeatureTextureView` with `PropertyTextureView`, which views a `PropertyTexture` in `EXT_structural_metadata`.
- Replaced `FeatureTexturePropertyView` with `PropertyTexturePropertyView`, which is a view of a `PropertyTextureProperty` in `EXT_structural_metadata`. This takes two template parameters: a typename `T` , and a `bool` indicating whether or not the values are normalized.
- Removed `FeatureTexturePropertyComponentType`, `FeatureTexturePropertyChannelOffsets`, and `FeatureTexturePropertyValue`. `PropertyTextureProperty` retrieves the values with the type indicated by its class property.
- Replaced `FeatureTexturePropertyViewStatus` with `PropertyTexturePropertyViewStatus`. `PropertyTexturePropertyViewStatus` is a class that inherits from `PropertyViewStatus`, defining additional error codes in the form of `static const` values.
- Renamed `FeatureIDTextureViewStatus` to `FeatureIdTextureViewStatus` for consistency.
- Renamed `MetadataArrayView` to `PropertyArrayView`.
- Renamed `FeatureTextureViewStatus` to `PropertyTextureViewStatus`.
- Refactored `PropertyType` to reflect the values of `type` in a `ClassProperty` from `EXT_structural_metadata`.

##### Additions :tada:

- Added `PropertyView`, which acts as a base class for all metadata property views. This takes two template parameters: a type `T` , and a `bool` indicating whether or not the values are normalized.
- Added `PropertyViewStatus`, which defines public `static const` values for various property errors.
- Added `PropertyTableViewStatus` to indicate whether a `PropertyTableView` is valid.
- Added `PropertyComponentType` to reflect the values of `componentType` in a `ClassProperty` from `EXT_structural_metadata`.
- Added `PropertyAttributeView`, which views a `PropertyAttribute` in `EXT_structural_metadata`.
- Added `PropertyAttributePropertyView`, which views a `PropertyAttributeProperty` in `EXT_structural_metadata`.
- Added `PropertyAttributePropertyViewStatus`, which reflects the status of a `PropertyAttributePropertyView`.

### v0.27.3 - 2023-10-01

##### Additions :tada:

- Added support for Cesium ion `"externalType"` assets.

##### Fixes :wrench:

- Fixed corner cases where `Tileset::ComputeLoadProgress` can incorrectly report done (100%) before all tiles are actually loaded for the current view.

### v0.27.2 - 2023-09-20

##### Additions :tada:

- Added `CESIUM_GLM_STRICT_ENABLED` option to the CMake scripts. It is ON by default, but when set to OFF it disables the `GLM_FORCE_XYZW_ONLY`, `GLM_FORCE_EXPLICIT_CTOR`, and `GLM_FORCE_SIZE_T_LENGTH` options in the GLM library.

##### Fixes :wrench:

- Added a missing include to `FeatureTexturePropertyView.h`.
- The CMake scripts no longer attempt to add the `Catch2` subdirectory when the tests are disabled.

### v0.27.1 - 2023-09-03

##### Fixes :wrench:

- Fixed a bug that could cause a crash when loading tiles with a raster overlay.

### v0.27.0 - 2023-09-01

##### Breaking Changes :mega:

- Renamed `ExtensionReaderContext` to `JsonReaderOptions`, and the `getExtensions` method on various JSON reader classes to `getOptions`.
- `IExtensionJsonHandler` no longer derives from `IJsonHandler`. Instead, it has a new pure virtual method, `getHandler`, that must be implemented to allow clients to obtain the `IJsonHandler`. In almost all implementations, this should simply return `*this`.
- In `SubtreeReader`, `SchemaReader`, and `TilesetReader`, the `readSubtree`, `readSchema`, and `readTileset` methods (respectively) have been renamed to `readFromJson` and return a templated `ReadJsonResult` instead of a bespoke result class.
- `TileExternalContent` is now heap allocated and stored in `TileContent` with a `std::unique_ptr`.
- The root `Tile` of a `Cesium3DTilesSelection::Tileset` now represents the tileset.json itself, and the `root` tile specified in the tileset.json is its only child. This makes the shape of the tile tree consistent between a standard top-level tileset and an external tileset embedded elsewhere in the tree. In both cases, the "tile" that represents the tileset.json itself has content of type `TileExternalContent`.

##### Additions :tada:

- Added new constructors to `LocalHorizontalCoordinateSystem` taking ECEF<->Local transformation matrices directly.
- Unknown properties in objects read with a `JsonReader` are now stored in the `unknownProperties` property on `ExtensibleObject` by default. To ignore them, as was done in previous versions, call `setCaptureUnknownProperties` on `JsonReaderOptions`.
- Added `ValueType` type alias to `ArrayJsonHandler`, for consistency with other JSON handlers.
- Added an overload of `JsonReader::readJson` that takes a `rapidjson::Value` instead of a byte buffer. This allows a subtree of a `rapidjson::Document` to be easily and efficiently converted into statically-typed classes via `IJsonHandler`.
- Added `*Reader` classes to `CesiumGltfReader` and `Cesium3DTilesReader` to allow each of the classes to be individually read from JSON.
- Added `getExternalContent` method to the `TileContent` class.
- `TileExternalContent` now holds the metadata (`schema`, `schemaUri`, `metadata`, and `groups`) stored in the tileset.json.
- Added `loadMetadata` and `getMetadata` methods to `Cesium3DTilesSelection::Tileset`. They provide access to `TilesetMetadata` instance representing the metadata associated with a tileset.json.
- Added `MetadataQuery` class to make it easier to find properties with specific semantics in `TilesetMetadata`.

##### Fixes :wrench:

- Fixed a bug where an empty error message would get propagated to a tileset's `loadErrorCallback`.
- Fixed several small build script issues to allow cesium-native to be used in Univeral Windows Platform (UWP) applications, such as those that run on Holo Lens 2.
- When KTX2 transcoding fails, the image will now be fully decompressed instead of returning an error.
- Fixed a bug that could cause higher-detail tiles to continue showing when zooming out quickly on a tileset that uses "additive" refinement.
- Fixed a bug that could cause a tile to never finish upsampling because its non-rendered parent never finishes loading.

### v0.26.0 - 2023-08-01

##### Additions :tada:

- Added caching support for Google Maps Photorealistic 3D Tiles. Or other cases where the origin server is using combinations of HTTP header directives that previously caused tiles not to go to disk cache (such as `max-age-0`, `stale-while-revalidate`, and `Expires`).
- Added support for the `EXT_meshopt_compression` extension, which allows decompressing mesh data using the meshoptimizer library. Also added support for the `KHR_mesh_quantization` and `KHR_texture_transform` extensions, which are often used together with the `EXT_meshopt_compression` extension to optimize the size and performance of glTF files.

##### Fixes :wrench:

- Fixed a bug in the 3D Tiles selection algorithm that could cause missing detail if a tileset had a leaf tile that was considered "unconditionally refined" due to having a geometric error larger than its parent's.
- Fixed a bug where `GltfReader::readImage` would always populate `mipPositions` when reading KTX2 images, even when the KTX2 file indicated that it had no mip levels and that they should be created, if necessary, from the base image. As a result, `generateMipMaps` wouldn't generate any mipmaps for the image.

### v0.25.1 - 2023-07-03

##### Additions :tada:

- Included generated glTF and 3D Tiles classes in the generated referenced documentation.
- Updated the 3D Tiles class generator to use the `main` branch instead of the `draft-1.1` branch.

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
- Added a new parameter to `IPrepareRendererResources::prepareInLoadThread`, `rendererOptions`, to allow passing arbitrary data from the renderer.

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
- Removed many methods from the `Cesium3DTilesSelection::Tileset` class: `getUrl()`, `getIonAssetID()`, `getIonAssetToken()`, `notifyTileStartLoading`, `notifyTileDoneLoading()`, `notifyTileUnloading()`, `loadTilesFromJson()`, `requestTileContent()`, `requestAvailabilitySubtree()`, `addContext()`, and `getGltfUpAxis()`. Most of these were already not recommended for use outside of cesium-native.
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
