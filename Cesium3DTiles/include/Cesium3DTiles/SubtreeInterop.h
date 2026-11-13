#pragma once

#include <CesiumUtility/Result.h>

namespace CesiumGltf {
struct Availability;
struct Buffer;
struct BufferView;
struct Model;
} // namespace CesiumGltf

namespace CesiumTilesetJson {
struct Availability;
struct Buffer;
struct BufferView;
struct Subtree;
} // namespace CesiumTilesetJson

namespace Cesium3DTiles {

/**
 * @brief Supports conversion of subtrees between 3D Tiles 1.1 and 3D Tiles 2.0
 * (glTF 2.1).
 */
class SubtreeInterop {
public:
  /**
   * @brief Converts a @ref CesiumTilesetJson::Buffer used for subtrees to a
   * @ref CesiumGltf::Buffer.
   *
   * @param buffer The @ref CesiumTilesetJson::Buffer to convert.
   * @returns The converted buffer.
   */
  static CesiumGltf::Buffer convertBuffer(CesiumTilesetJson::Buffer&& buffer);

  /**
   * @brief Converts a @ref CesiumGltf::Buffer to a @ref
   * CesiumTilesetJson::Buffer used for subtrees.
   *
   * @param buffer The @ref CesiumGltf::Buffer to convert.
   * @returns The conversion result with potential warnings or errors.
   */
  static CesiumUtility::Result<CesiumTilesetJson::Buffer>
  convertBuffer(CesiumGltf::Buffer&& buffer);

  /**
   * @brief Converts a @ref CesiumTilesetJson::BufferView used for subtrees to a
   * @ref CesiumGltf::BufferView.
   *
   * @param bufferView The @ref CesiumTilesetJson::BufferView to convert.
   * @returns The conversion result with potential warnings or errors.
   */
  static CesiumUtility::Result<CesiumGltf::BufferView>
  convertBufferView(CesiumTilesetJson::BufferView&& bufferView);

  /**
   * @brief Converts a @ref CesiumGltf::BufferView to a @ref
   * CesiumTilesetJson::BufferView used for subtrees.
   *
   * @param bufferView The @ref CesiumGltf::BufferView to convert.
   * @returns The conversion result with potential warnings or errors.
   */
  static CesiumUtility::Result<CesiumTilesetJson::BufferView>
  convertBufferView(CesiumGltf::BufferView&& bufferView);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Availability to a
   * @ref CesiumGltf::Availability.
   *
   * @param availability The CesiumTilesetJson::Availability to convert.
   * @returns The converted availability.
   */
  static CesiumGltf::Availability
  convertAvailability(CesiumTilesetJson::Availability&& availability);

  /**
   * @brief Converts a @ref CesiumGltf::Availability to a @ref
   * CesiumTilesetJson::Availability.
   *
   * @param availability The @ref CesiumGltf::Availability to convert.
   * @returns The converted availability.
   */
  static CesiumTilesetJson::Availability
  convertAvailability(CesiumGltf::Availability&& availability);

  /**
   * @brief Converts a @ref CesiumTilesetJson::Subtree to an equivalent @ref
   * CesiumGltf::Model with the @ref CesiumGltf::Extension3dTilesSubtree
   * extension.
   *
   * This attempts to merge all of @ref
   * CesiumTilesetJson::Subtree::contentAvailability into one
   * @ref CesiumGltf::Availability on @ref
   * CesiumGltf::Extension3dTilesSubtree::contentAvailability. Therefore, all
   * @ref CesiumTilesetJson::Subtree::buffers should be resolved and loaded into
   * memory to avoid conversion errors.
   *
   * @param subtree The @ref CesiumTilesetJson::Subtree to convert.
   * @returns The conversion result with potential warnings or errors.
   */
  static CesiumUtility::Result<CesiumGltf::Model>
  convertToGltfModel(CesiumTilesetJson::Subtree&& subtree);

  /**
   * @brief Converts a @ref CesiumGltf::Model with the @ref
   * CesiumGltf::Extension3dTilesSubtree extension to an equivalent @ref
   * CesiumTilesetJson::Subtree.
   *
   * @param model The @ref CesiumGltf::Model to convert.
   * @returns The conversion result with potential warnings or errors.
   */
  static CesiumUtility::Result<CesiumTilesetJson::Subtree>
  convertToTilesetJsonSubtree(CesiumGltf::Model&& model);
};
} // namespace Cesium3DTiles
