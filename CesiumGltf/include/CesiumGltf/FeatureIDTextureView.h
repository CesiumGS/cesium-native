#pragma once

#include "CesiumGltf/FeatureIDTexture.h"
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

/**
 * @brief The status of a feature id texture view.
 *
 * The {@link FeatureIDTextureView} constructor always completes successfully,
 * but it may not always reflect the actual content of the
 * {@link FeatureIDTexture}. This enumeration provides the reason.
 */
enum class FeatureIDTextureViewStatus {
  /**
   * @brief This view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This view has not yet been initialized.
   */
  InvalidUninitialized,

  /**
   * @brief This feature id texture has a texture index that doesn't exist in
   * the glTF.
   */
  InvalidTextureIndex,

  /**
   * @brief This feature id texture has an image index that doesn't exist in
   * the glTF.
   */
  InvalidImageIndex,

  /**
   * @brief This feature id texture has an unknown image channel.
   */
  InvalidChannel,

  /**
   * @brief This feature id texture has an empty image.
   */
  InvalidEmptyImage
};

/**
 * @brief A view on the image data of {@link FeatureIDTexture}.
 *
 * It provides the ability to sample the feature IDs from the
 * {@link FeatureIDTexture} using texture coordinates.
 */
class FeatureIDTextureView {
public:
  /**
   * @brief Constructs an uninitialized and invalid view.
   */
  FeatureIDTextureView() noexcept;

  /**
   * @brief Construct a view of the data specified by a
   * {@link FeatureIDTexture}.
   *
   * @param model The glTF in which to look for the feature id texture's data.
   * @param featureIDTexture The feature id texture to create a view for.
   */
  FeatureIDTextureView(
      const Model& model,
      const FeatureIDTexture& featureIDTexture) noexcept;

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
  int64_t getFeatureID(double u, double v) const noexcept;

  /**
   * @brief Get the status of this view.
   *
   * If invalid, it will not be safe to sample feature ids from this view.
   */
  FeatureIDTextureViewStatus status() const { return _status; }

  /**
   * @brief Get the actual feature ID texture.
   *
   * This will be nullptr if the feature id texture view runs into problems
   * during construction.
   */
  const ImageCesium* getImage() const { return _pImage; }

  /**
   * @brief Get the channel index that this feature ID texture uses.
   */
  int32_t getChannel() const { return _channel; }

  /**
   * @brief Get the name of the feature table associated with this feature ID
   * texture.
   */
  const std::string& getFeatureTableName() const {
    return this->_featureTableName;
  }

  /**
   * @brief Get the texture coordinate attribute index for this feature id
   * texture.
   */
  int64_t getTextureCoordinateAttributeId() const {
    return this->_textureCoordinateAttributeId;
  }

private:
  const ImageCesium* _pImage;
  int32_t _channel;
  int64_t _textureCoordinateAttributeId;
  std::string _featureTableName;
  FeatureIDTextureViewStatus _status;
};

} // namespace CesiumGltf