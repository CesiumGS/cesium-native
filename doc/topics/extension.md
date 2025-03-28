# Supported Extensions {#supported-extensions}

glTF and 3D Tiles allow their base specifications to be extended with new features. Cesium Native supports a number of extensions across both glTF and 3D Tiles.

## 3D Tiles Extensions

### 3DTILES_bounding_volume_cylinder

> This extension defines a bounding volume type for a region that follows the surface of a cylinder between two different radius values—aptly referred to as a "cylinder region". These regions are useful for visualizing real-world data that has been captured by cylindrical sensors.

Read more: https://github.com/CesiumGS/3d-tiles/tree/voxels/extensions/3DTILES_bounding_volume_cylinder

Cylinder bounding volumes are automatically handled when loading tiles using `Cesium3DTilesSelection::TilesetJsonLoader`. They can be explicitly manipulated using `Cesium3DTilesContent::TileBoundingVolumes::getBoundingCylinderRegion`, `Cesium3DTilesContent::TileBoundingVolumes::setBoundingCylinderRegion`, and `Cesium3DTilesContent::ImplicitTilingUtilities::computeBoundingVolume`. 

### 3DTILES_bounding_volume_S2

- *Implemented in: `Cesium3DTilesContent::TileBoundingVolumes`, `Cesium3DTilesSelection::TilesetJsonLoader`*

> S2 is a geometry library that defines a framework for decomposing the unit sphere into a hierarchy of cells. A cell is a quadrilateral bounded by four geodesics. The S2 cell hierarchy has 6 root cells, that are obtained by projecting the six faces of a cube onto the unit sphere.
> 
> Typically, traditional GIS libraries rely on projecting map data from an ellipsoid onto a plane. For example, the Mercator projection is a cylindrical projection, where the ellipsoid is mapped onto a cylinder that is then unrolled as a plane. This method leads to increasingly large distortion in areas as you move further away from the equator. S2 makes it possible to split the Earth into tiles that have no singularities, have relatively low distortion and are nearly equal in size.
> 
> This extension to 3D Tiles enables using S2 cells as bounding volumes. Due to the properties of S2 described above, this extension is well suited for tilesets that span the whole globe.

Read more: https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_bounding_volume_S2

S2 bounding volumes are automatically handled when loading tiles using `Cesium3DTilesSelection::TilesetJsonLoader`. They can be explicitly manipulated using `Cesium3DTilesContent::TileBoundingVolumes::getS2CellBoundingVolume`, `Cesium3DTilesContent::TileBoundingVolumes::setS2CellBoundingVolume`, and `Cesium3DTilesContent::ImplicitTilingUtilities::computeBoundingVolume`. 

### Unimplemented Extensions

Code for reading and writing the following extensions is currently present in Cesium Native, but they are not yet used anywhere else:

- [`3DTILES_ellipsoid`](https://github.com/CesiumGS/3d-tiles/tree/492adb06b00870d9ee99b8d97c261a466783034c/extensions/3DTILES_ellipsoid)
- [`3DTILES_content_voxels`](https://github.com/CesiumGS/3d-tiles/tree/cesium-native/extensions/3DTILES_content_voxels)

## glTF Extensions

### KHR_texture_transform

> Many techniques can be used to optimize resource usage for a 3d scene. Chief among them is the ability to minimize the number of textures the GPU must load. To achieve this, many engines encourage packing many objects' low-resolution textures into a single large texture atlas. The region of the resulting atlas that corresponds with each object is then defined by vertical and horizontal offsets, and the width and height of the region.
> 
> To support this use case, this extension adds offset, rotation, and scale properties to textureInfo structures. These properties would typically be implemented as an affine transform on the UV coordinates.

Read more: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_transform/README.md

`KHR_texture_transform` is handled for glTFs when using the `CesiumGltfReader::GltfReader::postprocess` method, if `CesiumGltfReader::GltfReaderOptions::applyTextureTransform` is set to `true`. 

### EXT_meshopt_compression

> glTF files come with a variety of binary data - vertex attribute data, index data, morph target deltas, animation inputs/outputs - that can be a substantial fraction of the overall transmission size. To optimize for delivery size, general-purpose compression such as gzip can be used - however, it often doesn't capture some common types of redundancy in glTF binary data.
> 
> This extension provides a generic option for compressing binary data that is tailored to the common types of data seen in glTF buffers. The extension works on a bufferView level and as such is agnostic of how the data is used, supporting geometry (vertex and index data, including morph targets), animation (keyframe time and values) and other data, such as instance transforms for `EXT_mesh_gpu_instancing`.

Read more: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_meshopt_compression/README.md

`EXT_meshopt_compression` is handled for glTFs when using the `CesiumGltfReader::GltfReader::postprocess` method, if `CesiumGltfReader::GltfReaderOptions::decodeMeshOptData` is set to `true`. 

### KHR_mesh_quantization

> Vertex attributes are usually stored using `FLOAT` component type. However, this can result in excess precision and increased memory consumption and transmission size, as well as reduced rendering performance.
> 
> This extension expands the set of allowed component types for mesh attribute storage to provide a memory/precision tradeoff - depending on the application needs, 16-bit or 8-bit storage can be sufficient.

Read more: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_mesh_quantization/README.md

`KHR_mesh_quantization` is handled for glTFs when using the `CesiumGltfReader::GltfReader::postprocess` method, if `CesiumGltfReader::GltfReaderOptions::dequantizeMeshData` is set to `true`. 

### KHR_texture_basisu

> This extension adds the ability to specify textures using KTX v2 images with Basis Universal supercompression. An implementation of this extension can use such images as an alternative to the PNG or JPEG images available in glTF 2.0 for more efficient asset transmission and reducing GPU memory footprint. Furthermore, specifying mip map levels is possible.

Read more: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_basisu/README.md

`KHR_texture_basisu` is handled for glTFs when using the `CesiumGltfReader::GltfReader::postprocess` method, if `CesiumGltfReader::GltfReaderOptions::decodeEmbeddedImages` is set to `true`. 

### EXT_texture_webp

> This extension allows glTF assets to use WebP as a valid image format.
> 
> WebP is an image format developed and maintained by Google that provides superior lossless and lossy compression rates for images on the web. It is typically 26% smaller in size compared to PNGs and 25-34% smaller than comparable JPEG images.

Read more: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_texture_webp/README.md

`EXT_texture_webp` is handled for glTFs when using the `CesiumGltfReader::GltfReader::postprocess` method, if `CesiumGltfReader::GltfReaderOptions::decodeEmbeddedImages` is set to `true`. 

