#pragma once

#include <Cesium3DTiles/Library.h>

#include <rapidjson/document.h>

#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>

namespace Cesium3DTiles {

/**
 * @brief Utility for identifying file formats and content types supported in 3D
 * Tiles.
 */
class CESIUM3DTILES_API FileFormat {
public:
  /**
   * @brief Enumerates the supported file formats for 3D Tiles.
   */
  enum class CESIUM3DTILES_API Type {
    /**
     * @brief Indicates an unknown file format.
     */
    Unknown,
    /**
     * @brief Indicates a glTF model in JSON format.
     */
    GltfJson,
    /**
     * @brief Indicates a glTF model in binary (glb) format.
     */
    GltfBinary,
    /**
     * @brief Indicates a Batched 3D Model (b3dm) from 3D Tiles 1.0.
     */
    Batched3dModel,
    /**
     * @brief Indicates a Composite (cmpt) from 3D Tiles 1.0.
     */
    Composite,
    /**
     * @brief Indicates an Instanced 3D Model (i3dm) from 3D Tiles 1.0.
     */
    Instanced3DModel,
    /**
     * @brief Indicates a Point Cloud (pnts) from 3D Tiles 1.0.
     */
    PointCloud,
    /**
     * @brief Indicates a subtree in JSON format from 3D Tiles 1.1.
     */
    SubtreeJson,
    /**
     * @brief Indicates a subtree in binary (subt) format from 3D Tiles 1.1.
     */
    SubtreeBinary,
    /**
     * @brief A tileset.json.
     */
    TilesetJson,
    /**
     * @brief A quantized-mesh layer.json. Alhough quantized-mesh is distinct
     * from the 3D Tiles spec, cesium-native supports quantized-mesh by
     * converting it to 3D Tiles at runtime.
     */
    LayerJson,
    /**
     * @brief A GeoJSON file, supported in 3D Tiles through @ref
     * CesiumTilesetJson::ExtensionTilesetMaxarContentGeoJson.
     */
    GeoJson
  };

  /**
   * @brief Retrieves the specified number of bytes at the start of the given
   * buffer, and interprets them as a string magic identifier for the data.
   *
   * @param data The data buffer.
   * @param length The target length for the magic.
   *
   * @returns The bytes as a string magic identifier. If there are less bytes
   * than requested, returns an empty string.
   */
  static CESIUM3DTILES_API std::string
  getMagic(const std::span<const std::byte>& data, size_t length = 4);

  /**
   * @brief Gets the magic identifier associated with the given file format.
   * Returns an empty string if no magic exists.
   *
   * @param type The file format type.
   *
   * @returns The magic associated with the format, or an empty string if there
   * isn't one.
   */
  static CESIUM3DTILES_API std::string getMagicForType(Type type);

  /**
   * @brief Gets the file format type associated with the given magic
   * identifier. Returns @ref Type::Unknown if no type has the specified magic.
   *
   * @param magic The magic identifier.
   *
   * @returns The file format type associated with the magic, or @ref
   * Type::Unknown if there is no such type.
   */
  static CESIUM3DTILES_API Type getTypeFromMagic(const std::string& magic);

  /**
   * @brief Infers the file format that best fits the given JSON document based
   * on its available members.
   *
   * @param document The parsed JSON document.
   *
   * @returns The best-fitting file format, or @ref Type::Unknown if one could
   * not be deduced.
   */
  static CESIUM3DTILES_API Type
  getBestFittingFormat(rapidjson::Document& document);

  /**
   * @brief Infers the file format that best fits the given data buffer based on
   * its four-letter magic.
   *
   * @param data The binary data.
   *
   * @returns The best-fitting file format, or @ref Type::Unknown if one could
   * not be deduced.
   */
  static CESIUM3DTILES_API Type
  getBestFittingFormat(const std::span<const std::byte>& data);

private:
  static std::unordered_map<std::string, Type> _magicToType;
  static std::unordered_map<Type, std::string> _typeToMagic;
};

} // namespace Cesium3DTiles
