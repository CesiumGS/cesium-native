#pragma once
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Wrapping behaviour for texture coordinates
 * (textureDefinitionInfo.cmn.md, deprecated). */
enum class TextureWrap { None, Repeat, Mirror };

/** @brief Channel mask for a texture (textureDefinitionInfo.cmn.md,
 * deprecated). */
enum class TextureChannels { Rgb, Rgba };

/** @brief Image format for a texture set entry (textureSetDefinition.cmn.md).
 */
enum class TextureFormat { Jpg, Png, Dds, KtxEtc2, Ktx2 };

/** @brief Single image resource within a texture definition (image.cmn.md). */
struct CESIUMI3S_API Image {
  /** @brief Image identifier. */
  std::string id;
  /** @brief Image size (width = height = size) in pixels. */
  std::optional<uint32_t> size;
  /** @brief Ground resolution in world units per pixel. */
  std::optional<double> pixelInWorldUnits;
  /** @brief Relative URLs to the image data. One per texture encoding. */
  std::optional<std::vector<std::string>> href;
  /** @brief Byte offsets into a multi-texture bundle. */
  std::optional<std::vector<uint64_t>> byteOffset;
  /** @brief Byte lengths for multi-texture bundle entries. */
  std::optional<std::vector<uint64_t>> length;
};

// store.defaultTextureDefinition)

/** @brief Legacy texture definition (textureDefinitionInfo.cmn.md, deprecated
 * I3S 1.7). */
struct CESIUMI3S_API TextureDefinitionInfo {
  /** @brief MIME types for the texture encodings (e.g. "image/jpeg"). */
  std::vector<std::string> encoding;
  /** @brief Wrapping behaviour for texture coordinates. */
  std::optional<std::vector<TextureWrap>> wrap;
  /** @brief When true, the texture is a texture atlas. */
  std::optional<bool> atlas;
  /** @brief Name of the UV set to use. */
  std::optional<std::string> uvSet;
  /** @brief Channel mask. */
  std::optional<TextureChannels> channels;
  /** @brief Image resources. */
  std::optional<std::vector<Image>> images;
};

/** @brief A single format entry in a texture set definition
 * (textureSetDefinition.cmn.md). */
struct CESIUMI3S_API TextureSetDefinitionFormat {
  /** @brief Name identifying this format entry. */
  std::string name;
  /** @brief Image format. */
  TextureFormat format = TextureFormat::Jpg;
};

/** @brief Texture set definition for I3S 1.7+ (textureSetDefinition.cmn.md). */
struct CESIUMI3S_API TextureSetDefinition {
  /** @brief Available image formats for this texture set. */
  std::vector<TextureSetDefinitionFormat> formats;
  /** @brief When true, the texture is a texture atlas. */
  std::optional<bool> atlas;
};

} // namespace CesiumI3S
