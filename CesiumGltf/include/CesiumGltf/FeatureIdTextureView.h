#pragma once

#include "CesiumGltf/FeatureIdTexture.h"
#include "CesiumGltf/Image.h"
#include "CesiumGltf/ImageCesium.h"
#include "CesiumGltf/KhrTextureTransform.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/Texture.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

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
   * @brief This feature ID texture has a sampler index that doesn't exist in
   * the glTF.
   */
  ErrorInvalidSampler,

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
   * @brief Construct a view of the data specified by a {@link FeatureIdTexture}.
   *
   * A feature ID texture may contain the `KHR_texture_transform` extension,
   * which transforms the texture coordinates used to sample the texture. The
   * extension may also override the TEXCOORD set index that was originally
   * specified by the feature ID texture.
   *
   * If a view is constructed with applyKhrTextureTransformExtension set to
   * true, the view will automatically apply the texture transform to any UV
   * coordinates used to sample the texture. If the extension defines its own
   * TEXCOORD set index, it will override the original value.
   *
   * Otherwise, if the flag is set to false, UVs will not be transformed and
   * the original TEXCOORD set index will be preserved. The extension's values
   * may still be retrieved using getTextureTransform, if desired.
   *
   * @param model The glTF in which to look for the feature ID texture's data.
   * @param featureIdTexture The feature ID texture to create a view for.
   * @param applyKhrTextureTransformExtension Whether to automatically apply the
   * `KHR_texture_transform` extension to the feature ID texture, if it exists.
   */
  FeatureIdTextureView(
      const Model& model,
      const FeatureIdTexture& featureIdTexture,
      bool applyKhrTextureTransformExtension = false) noexcept;

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
  FeatureIdTextureViewStatus status() const noexcept { return this->_status; }

  /**
   * @brief Get the actual feature ID texture.
   *
   * This will be nullptr if the feature ID texture view runs into problems
   * during construction.
   */
  const ImageCesium* getImage() const noexcept { return this->_pImage; }

  /**
   * @brief Get the sampler describing how to sample the data from the
   * feature ID texture.
   *
   * This will be nullptr if the feature ID texture view runs into
   * problems during construction.
   */
  const Sampler* getSampler() const noexcept { return this->_pSampler; }

  /**
   * @brief Get the channels of this feature ID texture. The channels represent
   * the bytes of the actual feature ID, in little-endian order.
   */
  std::vector<int64_t> getChannels() const noexcept { return this->_channels; }

  /**
   * @brief Get the texture coordinate set index for this feature ID
   * texture.
   *
   * If applyKhrTextureTransformExtension is true, and if the feature ID texture
   * contains the `KHR_texture_transform` extension, this will return the value
   * from the extension, as it is meant to override the original index. However,
   * if the extension does not specify a TEXCOORD set index, then the original
   * index of the feature ID texture is returned.
   */
  int64_t getTexCoordSetIndex() const noexcept {
    if (this->_applyTextureTransform && this->_textureTransform) {
      return this->_textureTransform->getTexCoordSetIndex().value_or(
          this->_texCoordSetIndex);
    }
    return this->_texCoordSetIndex;
  }

  /**
   * @brief Get the KHR_texture_transform for this feature ID texture, if it
   * exists.
   *
   * Even if this view was constructed with applyKhrTextureTransformExtension
   * set to false, it will save the extension's values, and they may be
   * retrieved through this function.
   *
   * If this view was constructed with applyKhrTextureTransformExtension set to
   * true, any texture coordinates passed into `getFeatureID` will be
   * automatically transformed, so there's no need to re-apply the transform
   * here.
   */
  std::optional<KhrTextureTransform> getTextureTransform() const noexcept {
    return this->_textureTransform;
  }

private:
  FeatureIdTextureViewStatus _status;
  int64_t _texCoordSetIndex;
  std::vector<int64_t> _channels;

  const ImageCesium* _pImage;
  const Sampler* _pSampler;

  bool _applyTextureTransform;
  std::optional<KhrTextureTransform> _textureTransform;
};
} // namespace CesiumGltf
