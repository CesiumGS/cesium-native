#pragma once

#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureIdTexture.h"
#include "CesiumGltf/Texture.h"
#include "CesiumGltf/TextureAccessor.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace CesiumGltf {
namespace MeshFeatures {

/**
 * @brief The status of a feature ID texture view.
 *
 * The {@link FeatureIdTextureView} constructor always completes successfully,
 * but it may not always reflect the actual content of the
 * {@link ExtensionExtMeshFeaturesFeatureIdTexture}. This enumeration provides the reason.
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
   * than a byte. The feature ID texture's channels should represent the bytes
   * of the actual feature ID.
   */
  ErrorInvalidImageChannelSize,

  /**
   * @brief This feature ID texture has a negative TEXCOORD set index.
   */
  ErrorInvalidTexCoordSetIndex,

  /**
   * @brief The channels of this feature ID texture property are invalid.
   * Channels must be in the range 0-3, with a minimum of one channel. Although
   * more than four channels can be defined for specialized texture
   * formats, this view only supports a maximum of four channels.
   */
  ErrorInvalidChannels
};

/**
 * @brief A view on the image data of {@link ExtensionExtMeshFeaturesFeatureIdTexture}.
 *
 * It provides the ability to sample the feature IDs from the
 * {@link ExtensionExtMeshFeaturesFeatureIdTexture} using texture coordinates.
 */
class FeatureIdTextureView {
public:
  /**
   * @brief Constructs an uninitialized and invalid view.
   */
  FeatureIdTextureView() noexcept;

  /**
   * @brief Construct a view of the data specified by a
   * {@link ExtensionExtMeshFeaturesFeatureIdTexture}.
   *
   * @param model The glTF in which to look for the feature id texture's data.
   * @param featureIDTexture The feature id texture to create a view for.
   */
  FeatureIdTextureView(
      const Model& model,
      const ExtensionExtMeshFeaturesFeatureIdTexture&
          featureIdTexture) noexcept;

  /**
   * @brief Get the Feature ID for the given texture coordinates.
   *
   * Will return -1 when the status is not Valid.
   *
   * @param u The u-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @param v The v-component of the texture coordinates. Must be within
   * [0.0, 1.0].
   * @return The feature ID at the nearest pixel to the texture coordinates.
   */
  int64_t getFeatureId(double u, double v) const noexcept;

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
  std::vector<int64_t> _channels;
  int64_t _texCoordSetIndex;

  const ImageCesium* _pImage;
};

} // namespace MeshFeatures
} // namespace CesiumGltf
