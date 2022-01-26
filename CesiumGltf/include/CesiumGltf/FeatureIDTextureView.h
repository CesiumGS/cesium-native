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

enum class FeatureIDTextureViewStatus {
  Valid,
  InvalidUninitialized,
  InvalidTextureIndex,
  InvalidImageIndex,
  InvalidChannel,
  InvalidEmptyImage
};

class FeatureIDTextureView {
public:
  FeatureIDTextureView() noexcept;

  FeatureIDTextureView(
      const Model& model,
      const FeatureIDTexture& featureIDTexture) noexcept;

  /**
   * @brief Get the Feature ID for the given texture coordinates.
   *
   * Not safe when the status is not Valid.
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
  constexpr FeatureIDTextureViewStatus status() const { return _status; }

  /**
   * @brief Get the actual feature ID texture.
   */
  constexpr const ImageCesium* getImage() const { return _pImage; }

  /**
   * @brief Get the channel index that this feature ID texture uses.
   */
  constexpr int32_t getChannel() const { return _channel; }

  /**
   * @brief Get the name of the feature table associated with this feature ID
   * texture.
   */
  constexpr const std::string& getFeatureTableName() const {
    return this->_featureTableName;
  }

  /**
   * @brief Get the texture coordinate index for this feature id texture.
   */
  constexpr int64_t getTextureCoordinateIndex() const {
    return this->_textureCoordinateIndex;
  }

private:
  const ImageCesium* _pImage;
  int32_t _channel;
  int64_t _textureCoordinateIndex;
  std::string _featureTableName;
  FeatureIDTextureViewStatus _status;
};

} // namespace CesiumGltf