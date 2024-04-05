#pragma once

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Model.h>

namespace Cesium3DTilesContent {

/**
 * @brief Creates a new glTF model from one of the quadtree children of the
 * given parent model.
 *
 * The parent model subdivision is guided by texture coordinates. These texture
 * coordinates must follow a map projection, and the parent tile is divided into
 * quadrants as divided by this map projection.
 *
 * @param parentModel The parent model to upsample.
 * @param childID The quadtree tile ID of the child model. This is used to
 * determine which of the four children of the parent tile to generate.
 * @param hasInvertedVCoordinate True if the V texture coordinate has 0.0 as the
 * Northern-most coordinate; False if the V texture coordinate has 0.0 as the
 * Southern-most coordiante.
 * @param textureCoordinateAttributeBaseName The base name of the attribute that
 * holds the projected texture coordinates. The `textureCoordinateIndex` is
 * appended to this name.
 * @param textureCoordinateIndex The index of the texture coordinate set to use.
 * For example, if `textureCoordinateAttributeBaseName` is `TEXCOORD_` and this
 * parameter is 4, then the texture coordinates are read from a vertex attribute
 * named `TEXCOORD_4`.
 * @return The upsampled model.
 */
std::optional<CesiumGltf::Model> upsampleGltfForRasterOverlays(
    const CesiumGltf::Model& parentModel,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    bool hasInvertedVCoordinate = false,
    const std::string& textureCoordinateAttributeBaseName = "TEXCOORD_",
    int32_t textureCoordinateIndex = 0);

} // namespace Cesium3DTilesContent
