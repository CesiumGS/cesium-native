#pragma once

#include "CesiumGltf/FeatureIdTexture.h"
#include "CesiumGltf/Texture.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace CesiumGltf {
/**
 * @brief The status of a feature ID texture view.
 *
 * The {@link FeatureIdTextureView} constructor always completes successfully,
 * but it may not always reflect the actual content of the
 * {@link FeatureIdTexture}. This enumeration provides the reason.
 */
enum class FeatureIdTextureViewStatus {
  /**
   * @brief This view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This view has not yet been initialized.
   */
  ErrorUninitialized,

  /**
   * @brief This feature ID texture has a texture index that doesn't exist in
   * the glTF.
   */
  ErrorInvalidTexture,

  /**
   * @brief This feature ID texture has an image index that doesn't exist in
   * the glTF.
   */
  ErrorInvalidImage,

  /**
   * @brief This feature ID texture has an empty image.
   */
  ErrorEmptyImage,

  /**
   * @brief The image for this feature ID texture has channels that take up more
   * than a byte. Only single-byte channels are supported.
   */
  ErrorInvalidImageBytesPerChannel,

  /**
   * @brief The channels of this feature ID texture property are invalid.
   * Channels must be in the range 0-3, with a minimum of one channel. Although
   * more than four channels can be defined for specialized texture
   * formats, this view only supports a maximum of four channels.
   */
  ErrorInvalidChannels
};

/**
 * @brief A view on the image data of {@link FeatureIdTexture}.
 *
 * It provides the ability to sample the feature IDs from the
 * {@link FeatureIdTexture} using texture coordinates.
 */
class FeatureIdTextureView {
public:
  /**
   * @brief Constructs an uninitialized and invalid view.
   */
  FeatureIdTextureView() noexcept;

  /**
   * @brief Construct a view of the data specified by a
   * {@link FeatureIdTexture}.
   *
   * @param model The glTF in which to look for the feature ID texture's data.
   * @param featureIDTexture The feature ID texture to create a view for.
   */
  FeatureIdTextureView(
      const Model& model,
      const FeatureIdTexture& featureIdTexture) noexcept;

  /**
   * @brief Get the feature ID from the texture at the given texture
   * coordinates. If the texture is somehow invalid, this returns -1.
   *
   * @param u The u-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @param v The v-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @return The feature ID at the nearest pixel to the texture coordinates.
   */
  int64_t getFeatureID(double u, double v) const noexcept;

  /**
   * @brief Get the status of this view.
   *
   * If invalid, it will not be safe to sample feature IDs from this view.
   */
  FeatureIdTextureViewStatus status() const { return _status; }

  /**
   * @brief Get the actual feature ID texture.
   *
   * This will be nullptr if the feature ID texture view runs into problems
   * during construction.
   */
  const ImageCesium* getImage() const { return _pImage; }

  /**
   * @brief Get the channels of this feature ID texture. The channels represent
   * the bytes of the actual feature ID, in little-endian order.
   */
  std::vector<int64_t> getChannels() const { return _channels; }

  /**
   * @brief Get the texture coordinate set index for this feature ID
   * texture.
   */
  int64_t getTexCoordSetIndex() const { return this->_texCoordSetIndex; }

private:
  FeatureIdTextureViewStatus _status;
  int64_t _texCoordSetIndex;
  std::vector<int64_t> _channels;

  const ImageCesium* _pImage;
};
} // namespace CesiumGltf
