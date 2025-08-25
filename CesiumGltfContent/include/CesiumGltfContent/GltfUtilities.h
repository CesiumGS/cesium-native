#pragma once

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltfContent/Library.h>

#include <glm/fwd.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumGltf {
struct Buffer;
struct Model;
struct Node;
} // namespace CesiumGltf

namespace CesiumGeometry {
class Ray;
} // namespace CesiumGeometry

namespace CesiumGltfContent {
/**
 * A collection of utility functions that are used to process and transform a
 * gltf model
 */
struct CESIUMGLTFCONTENT_API GltfUtilities {
  /**
   * @brief Gets the transformation matrix for a given node.
   *
   * This returns the node's local transform as-is. It does not incorporate
   * transforms from any of the node's ancestors.
   *
   * @param node The node from which to get the transformation matrix.
   * @return The transformation matrix, or std::nullopt if the node's
   * transformation is invalid, .e.g, because it has a matrix with fewer than
   * 16 elements in it.
   */
  static std::optional<glm::dmat4x4>
  getNodeTransform(const CesiumGltf::Node& node);

  /**
   * @brief Sets the transformation matrix for a given node.
   *
   * This sets only the local transform of the node. It does not affect the
   * transforms of any ancestor or descendant nodes, if present.
   *
   * @param node The node on which to set the transformation matrix.
   * @param newTransform The new transformation matrix.
   */
  static void
  setNodeTransform(CesiumGltf::Node& node, const glm::dmat4x4& newTransform);

  /**
   * @brief Applies the glTF's RTC_CENTER, if any, to the given transform.
   *
   * If the glTF has a `CESIUM_RTC` extension, this function will multiply the
   * given matrix with the (translation) matrix that is created from the
   * `RTC_CENTER` in the. If the given model does not have this extension, then
   * this function will return the `rootTransform` unchanged.
   *
   * @param gltf The glTF model
   * @param rootTransform The matrix that will be multiplied with the transform
   * @return The result of multiplying the `RTC_CENTER` with the
   * `rootTransform`.
   */
  static glm::dmat4x4 applyRtcCenter(
      const CesiumGltf::Model& gltf,
      const glm::dmat4x4& rootTransform);

  /**
   * @brief Applies the glTF's `gltfUpAxis`, if any, to the given transform.
   *
   * By default, the up-axis of a glTF model will the the Y-axis.
   *
   * If the tileset that contained the model had the `asset.gltfUpAxis` string
   * property, then the information about the up-axis has been stored in as a
   * number property called `gltfUpAxis` in the `extras` of the given model.
   *
   * Depending on whether this value is `CesiumGeometry::Axis::X`, `Y`, or `Z`,
   * the given matrix will be multiplied with a matrix that converts the
   * respective axis to be the Z-axis, as required by the 3D Tiles standard.
   *
   * @param model The glTF model
   * @param rootTransform The matrix that will be multiplied with the transform
   * @return The result of multiplying the `rootTransform` with the
   * `gltfUpAxis`.
   */
  static glm::dmat4x4 applyGltfUpAxisTransform(
      const CesiumGltf::Model& model,
      const glm::dmat4x4& rootTransform);

  /**
   * @brief Computes a bounding region from the vertex positions in a glTF
   * model.
   *
   * If the glTF model spans the anti-meridian, the west and east longitude
   * values will be in the usual -PI to PI range, but east will have a smaller
   * value than west.
   *
   * If the glTF contains no geometry, the returned region's rectangle
   * will be {@link CesiumGeospatial::GlobeRectangle::EMPTY}, its minimum height will be 1.0, and
   * its maximum height will be -1.0 (the minimum will be greater than the
   * maximum).
   *
   * @param gltf The model.
   * @param transform The transform from model coordinates to ECEF coordinates.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   * @return The computed bounding region.
   */
  static CesiumGeospatial::BoundingRegion computeBoundingRegion(
      const CesiumGltf::Model& gltf,
      const glm::dmat4& transform,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Parse the copyright field of a glTF model and return the individual
   * credits.
   *
   * Credits are read from the glTF's asset.copyright field. This method assumes
   * that individual credits are separated by semicolons.
   *
   * @param gltf The model.
   * @return The credits from the glTF.
   */
  static std::vector<std::string_view>
  parseGltfCopyright(const CesiumGltf::Model& gltf);

  /**
   * @brief Parse a semicolon-separated string, such as the copyright field of a
   * glTF model, and return the individual parts (credits).
   *
   * @param s The string to parse.
   * @return The semicolon-delimited parts.
   */
  static std::vector<std::string_view>
  parseGltfCopyright(const std::string_view& s);

  /**
   * @brief Merges all of the glTF's buffers into a single buffer (the first
   * one).
   *
   * This is useful when writing the glTF as a GLB, which supports only a single
   * embedded buffer.
   *
   * @param gltf The glTF in which to merge buffers.
   */
  static void collapseToSingleBuffer(CesiumGltf::Model& gltf);

  /**
   * @brief Copies the content of one {@link CesiumGltf::Buffer} to the end of another,
   * updates all {@link CesiumGltf::BufferView} instances to refer to the destination
   * buffer, and clears the contents of the original buffer.
   *
   * The source buffer is not removed, but it has a `byteLength` of zero after
   * this function completes.
   *
   * @param gltf The glTF model to modify.
   * @param destination The destination Buffer into which to move content.
   * @param source The source Buffer from which to move content.
   */
  static void moveBufferContent(
      CesiumGltf::Model& gltf,
      CesiumGltf::Buffer& destination,
      CesiumGltf::Buffer& source);

  /**
   * @brief Removes unused textures from the given glTF model.
   *
   * @param gltf The glTF to remove unused textures from.
   * @param extraUsedTextureIndices Indices of textures that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedTextures(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedTextureIndices = {});

  /**
   * @brief Removes unused samplers from the given glTF model.
   *
   * @param gltf The glTF to remove unused samplers from.
   * @param extraUsedSamplerIndices Indices of samplers that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedSamplers(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedSamplerIndices = {});

  /**
   * @brief Removes unused images from the given glTF model.
   *
   * @param gltf The glTF to remove unused images from.
   * @param extraUsedImageIndices Indices of images that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedImages(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedImageIndices = {});

  /**
   * @brief Removes unused accessors from the given glTF model.
   *
   * @param gltf The glTF to remove unused accessors from.
   * @param extraUsedAccessorIndices Indices of accessors that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedAccessors(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedAccessorIndices = {});

  /**
   * @brief Removes unused buffer views from the given glTF model.
   *
   * @param gltf The glTF to remove unused buffer views from.
   * @param extraUsedBufferViewIndices Indices of buffer views that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedBufferViews(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedBufferViewIndices = {});

  /**
   * @brief Removes unused buffers from the given glTF model.
   *
   * @param gltf The glTF to remove unused buffers from.
   * @param extraUsedBufferIndices Indices of buffers that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedBuffers(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedBufferIndices = {});

  /**
   * @brief Removes unused meshes from the given glTF model.
   *
   * @param gltf The glTF to remove unused meshes from.
   * @param extraUsedMeshIndices Indices of meshes that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedMeshes(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedMeshIndices = {});

  /**
   * @brief Removes unused materials from the given glTF model.
   *
   * @param gltf The glTF to remove unused materials from.
   * @param extraUsedMaterialIndices Indices of materials that should be
   * considered "used" even if they're not referenced by anything else in the
   * glTF.
   */
  static void removeUnusedMaterials(
      CesiumGltf::Model& gltf,
      const std::vector<int32_t>& extraUsedMaterialIndices = {});

  /**
   * @brief Shrink buffers by removing any sections that are not referenced by
   * any BufferView.
   *
   * @param gltf The glTF to modify.
   */
  static void compactBuffers(CesiumGltf::Model& gltf);

  /**
   * @brief Shrink a buffer by removing any sections that are not referenced by
   * any BufferView.
   *
   * @param gltf The glTF to modify.
   * @param bufferIndex The index of the buffer to compact.
   */
  static void compactBuffer(CesiumGltf::Model& gltf, int32_t bufferIndex);

  /**
   * @brief Data describing a hit from a ray / gltf intersection test
   */
  struct RayGltfHit {
    /**
     * @brief Hit point in primitive space
     */
    glm::dvec3 primitivePoint = {};

    /**
     * @brief Transformation from primitive to world space
     */
    glm::dmat4x4 primitiveToWorld = {};

    /**
     * @brief Hit point in world space
     */
    glm::dvec3 worldPoint = {};

    /**
     * @brief Square dist from intersection ray origin to world point
     */
    double rayToWorldPointDistanceSq = -1.0;

    /**
     * @brief ID of the glTF mesh that was hit
     */
    int32_t meshId = -1;

    /**
     * @brief ID of the glTF primitive that was hit
     */
    int32_t primitiveId = -1;
  };

  /**
   * @brief Hit result data for intersectRayGltfModel
   */
  struct IntersectResult {
    /**
     * @brief Optional hit result, if an intersection occurred
     */
    std::optional<RayGltfHit> hit;

    /**
     * @brief Warnings encountered when traversing the glTF model
     */
    std::vector<std::string> warnings{};
  };

  /**
   * @brief Intersects a ray with a glTF model and returns the first
   * intersection point.
   *
   * Supports all mesh primitive modes.
   * Points and lines are assumed to have no area, and are ignored
   *
   * @param ray A ray in world space.
   * @param gltf The glTF model to intersect.
   * @param cullBackFaces Ignore triangles that face away from ray. Front faces
   * use CCW winding order.
   * @param gltfTransform Optional matrix to apply to entire gltf model.
   * @returns IntersectResult describing outcome
   */
  static IntersectResult intersectRayGltfModel(
      const CesiumGeometry::Ray& ray,
      const CesiumGltf::Model& gltf,
      bool cullBackFaces = true,
      const glm::dmat4x4& gltfTransform = glm::dmat4(1.0));
};
} // namespace CesiumGltfContent
