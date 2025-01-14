#pragma once

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>

#include <glm/fwd.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace CesiumGltf {
struct Model;
}

namespace CesiumRasterOverlays {

/**
 * @brief A collection of utilities useful for operations involving raster
 * overlay tiles.
 */
struct CESIUMRASTEROVERLAYS_API RasterOverlayUtilities {
  /**
   * @brief Texture coordinates will be stored in the glTF under the following
   * name, appended with a number. See \ref
   * createRasterOverlayTextureCoordinates.
   */
  static constexpr std::string_view DEFAULT_TEXTURE_COORDINATE_BASE_NAME =
      "_CESIUMOVERLAY_";

  /**
   * @brief Creates texture coordinates for mapping {@link RasterOverlay} tiles
   * to a glTF model.
   *
   * Generates new texture coordinates for the `gltf` using the given
   * `projections`. The first new texture coordinate (`u` or `s`) will be 0.0 at
   * the `minimumX` of the given `rectangle` and 1.0 at the `maximumX`. The
   * second texture coordinate (`v` or `t`) will be 0.0 at the `minimumY` of
   * the given `rectangle` and 1.0 at the `maximumY`.
   *
   * Coordinate values for vertices in between these extremes are determined by
   * projecting the vertex position with the `projection` and then computing the
   * fractional distance of that projected position between the minimum and
   * maximum.
   *
   * Projected positions that fall outside the `globeRectangle` will be clamped
   * to the edges, so the coordinate values will never be less then 0.0 or
   * greater than 1.0.
   *
   * These texture coordinates are stored in the provided glTF, and a new
   * primitive attribute is added to each primitive for each projection. The new
   * primitive attributes are named with `textureCoordinateAttributeBaseName`
   * followed by a number, starting with `firstTextureCoordinateID` and
   * incrementing for each projection. For example, if
   * `textureCoordinateAttributeBaseName` is
   * `_CESIUMOVERLAY_` and `firstTextureCoordinateID` is 0 (the defaults), then
   * the texture coordinates for the first projection will be stored in an
   * attribute named `_CESIUMOVERLAY_0`, the second will be in
   * `_CESIUMOVERLAY_1`, and so on.
   *
   * @param gltf The glTF model.
   * @param modelToEcefTransform The transformation of this glTF to ECEF
   * coordinates.
   * @param globeRectangle The rectangle that all projected vertex positions are
   * expected to lie within. If this parameter is std::nullopt, it is computed
   * from the vertices.
   * @param projections The projections for which to generate texture
   * coordinates. There is a linear relationship between the coordinates of this
   * projection and the generated texture coordinates.
   * @param invertVCoordinate True if the V texture coordinate should be
   * inverted so that it is 1.0 at the Southern end of the rectangle and 0.0 at
   * the Northern end. This is useful with images that use the typical North-up
   * orientation.
   * @param textureCoordinateAttributeBaseName The base name to use for the
   * texture coordinate attributes, without a number on the end. Defaults to
   * {@link DEFAULT_TEXTURE_COORDINATE_BASE_NAME}.
   * @param firstTextureCoordinateID The texture coordinate ID of the first
   * projection.
   * @return The details of the generated texture coordinates.
   */
  static std::optional<RasterOverlayDetails>
  createRasterOverlayTextureCoordinates(
      CesiumGltf::Model& gltf,
      const glm::dmat4& modelToEcefTransform,
      const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
      std::vector<CesiumGeospatial::Projection>&& projections,
      bool invertVCoordinate = false,
      const std::string_view& textureCoordinateAttributeBaseName =
          DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
      int32_t firstTextureCoordinateID = 0);

  /**
   * @brief Creates a new glTF model from one of the quadtree children of the
   * given parent model.
   *
   * The parent model subdivision is guided by texture coordinates. These
   * texture coordinates must follow a map projection, and the parent tile is
   * divided into quadrants as divided by this map projection. To create the
   * necessary texture coordinates, use
   * {@link createRasterOverlayTextureCoordinates}.
   *
   * @param parentModel The parent model to upsample.
   * @param childID The quadtree tile ID of the child model. This is used to
   * determine which of the four children of the parent tile to generate.
   * @param hasInvertedVCoordinate True if the V texture coordinate has 0.0 as
   * the Northern-most coordinate; False if the V texture coordinate has 0.0 as
   * the Southern-most coordiante.
   * @param textureCoordinateAttributeBaseName The base name of the attribute
   * that holds the projected texture coordinates. The `textureCoordinateIndex`
   * is appended to this name. Defaults to
   * {@link DEFAULT_TEXTURE_COORDINATE_BASE_NAME}.
   * @param textureCoordinateIndex The index of the texture coordinate set to
   * use. For example, if `textureCoordinateAttributeBaseName` is
   * `_CESIUMOVERLAY_` and this parameter is 0 (the defaults), then the texture
   * coordinates are read from a vertex attribute named `_CESIUMOVERLAY_0`.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   * @return The upsampled model.
   */
  static std::optional<CesiumGltf::Model> upsampleGltfForRasterOverlays(
      const CesiumGltf::Model& parentModel,
      CesiumGeometry::UpsampledQuadtreeNode childID,
      bool hasInvertedVCoordinate = false,
      const std::string_view& textureCoordinateAttributeBaseName =
          DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
      int32_t textureCoordinateIndex = 0,
      const CesiumGeospatial::Ellipsoid& ellipsoid =
          CesiumGeospatial::Ellipsoid::WGS84);

  /**
   * @brief Computes the desired screen pixels for a raster overlay texture.
   *
   * This method is used to determine the appropriate number of "screen pixels"
   * to use for a raster overlay texture to be attached to a glTF (which is
   * usually a 3D Tiles tile). In other words, how detailed should the texture
   * be? The answer depends, of course, on how close we'll get to the model. If
   * we're going to get very close, we'll need a higher-resolution raster
   * overlay texture than if we will stay far away from it.
   *
   * In 3D Tiles, we can only get so close to a model before it switches to the
   * next higher level-of-detail by showing its children instead. The switch
   * distance is controlled by the `geometric error` of the tile, as well as by
   * the `maximum screen space error` of the tileset. So this method requires
   * both of those parameters.
   *
   * The answer also depends on the size of the model on the screen at this
   * switch distance. To determine that, this method takes a projection and a
   * rectangle that bounds the tile, expressed in that projection. This
   * rectangle is projected onto the screen at the switch distance, and the size
   * of that rectangle on the screen is the `target screen pixels` returned by
   * this method.
   *
   * The `target screen pixels` returned here may be further modified by the
   * raster overlay's {@link RasterOverlayTileProvider::getTile} method. In particular, it
   * will usually be divided by the raster overlay's `maximum screen space
   * error` of the raster overlay (not to be confused with the `maximum screen
   * space error` of the tileset, mentioned above).
   *
   * @param geometricError The geometric error of the tile.
   * @param maximumScreenSpaceError The maximum screen-space error used to
   * render the tileset.
   * @param projection The projection in which the `rectangle` parameter is
   * provided.
   * @param rectangle The 2D extent of the tile, expressed in the `projection`.
   * @param ellipsoid The ellipsoid with which computations are performed.
   * @return The desired screen pixels.
   */
  static glm::dvec2 computeDesiredScreenPixels(
      double geometricError,
      double maximumScreenSpaceError,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& rectangle,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Computes the texture translation and scale necessary to align a
   * raster overlay with the given rectangle on geometry whose texture
   * coordinates were computed using a different rectangle.
   *
   * @param geometryRectangle The geometry rectangle used to the compute the
   * texture coordinates.
   * @param overlayRectangle The rectangle covered by the raster overlay
   * texture.
   * @return The translation in X and Y, and the scale in Z and W.
   */
  static glm::dvec4 computeTranslationAndScale(
      const CesiumGeometry::Rectangle& geometryRectangle,
      const CesiumGeometry::Rectangle& overlayRectangle);
};

} // namespace CesiumRasterOverlays
