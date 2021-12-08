#pragma once

#include "ExtensionModelExtFeatureMetadata.h"
#include "FeatureIDTexture.h"
#include "FeatureTable.h"
#include "Image.h"
#include "ImageCesium.h"
#include "Model.h"
#include "Texture.h"
#include "TextureAccessor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace CesiumGltf {

enum class FeatureIDTextureViewStatus {
  Valid,
  InvalidUninitialized,
  InvalidExtensionMissing,
  InvalidFeatureTableMissing,
  InvalidTextureIndex,
  InvalidImageIndex,
  InvalidChannel
};

class FeatureIDTextureView {
public:
  FeatureIDTextureView() noexcept
      : _pImage(nullptr),
        _channel(0),
        _textureCoordinateIndex(-1),
        _pFeatureTable(nullptr),
        _status(FeatureIDTextureViewStatus::InvalidUninitialized) {}

  FeatureIDTextureView(
      const Model& model,
      const FeatureIDTexture& featureIDTexture) noexcept
      : _pImage(nullptr),
        _channel(0),
        _textureCoordinateIndex(-1),
        _pFeatureTable(nullptr),
        _status(FeatureIDTextureViewStatus::InvalidUninitialized) {

    const ExtensionModelExtFeatureMetadata* pExtension =
        model.getExtension<ExtensionModelExtFeatureMetadata>();

    if (!pExtension) {
      this->_status = FeatureIDTextureViewStatus::InvalidExtensionMissing;
      return;
    }

    auto tableIt =
        pExtension->featureTables.find(featureIDTexture.featureTable);
    if (tableIt == pExtension->featureTables.end()) {
      this->_status = FeatureIDTextureViewStatus::InvalidFeatureTableMissing;
      return;
    }

    this->_pFeatureTable = &tableIt->second;

    this->_textureCoordinateIndex =
        featureIDTexture.featureIds.texture.texCoord;

    int32_t textureIndex = featureIDTexture.featureIds.texture.index;
    if (textureIndex < 0 || textureIndex >= model.textures.size()) {
      this->_status = FeatureIDTextureViewStatus::InvalidTextureIndex;
      return;
    }

    const Texture& texture = model.textures[textureIndex];

    // Ignore sampler, we will always use nearest pixel sampling.
    if (texture.source < 0 || texture.source >= model.images.size()) {
      this->_status = FeatureIDTextureViewStatus::InvalidImageIndex;
      return;
    }

    this->_pImage = &model.images[texture.source].cesium;

    // TODO: is this right?
    // This assumes that if the channel is g, there must be at least two
    // channels (r and g). If it is b there must be r, g, and b. If there is a,
    // then there must be r, g, b, and a.
    // For instance, if the image has two channels, will those be referred to
    // as r and g or r and a?
    if (featureIDTexture.featureIds.channels == "r") {
      this->_channel = 0;
    } else if (featureIDTexture.featureIds.channels == "g") {
      this->_channel = 1;
    } else if (featureIDTexture.featureIds.channels == "b") {
      this->_channel = 2;
    } else if (featureIDTexture.featureIds.channels == "a") {
      this->_channel = 3;
    } else {
      this->_status = FeatureIDTextureViewStatus::InvalidChannel;
      return;
    }

    this->_status = FeatureIDTextureViewStatus::Valid;

    // TODO: add more validation steps, check if image has at least one pixel.

    // TODO: once compressed texture support is merged, check that the image is
    // decompressed here.
  }

  /**
   * @brief Get the actual feature ID texture.
   * @return The feature ID texture.
   */
  constexpr const ImageCesium* getImage() const { return _pImage; }

  /**
   * @brief Get the channel index that this feature ID texture uses.
   * @return The channel index;
   */
  constexpr int32_t getChannel() const { return _channel; }

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
  int64_t getFeatureID(double u, double v) const noexcept {
    assert(this->_status == FeatureIDTextureViewStatus::Valid);

    int64_t x = std::clamp(
        std::llround(u * this->_pImage->width),
        0LL,
        (int64_t)this->_pImage->width);
    int64_t y = std::clamp(
        std::llround(v * this->_pImage->height),
        0LL,
        (int64_t)this->_pImage->height);

    int64_t pixelOffset = this->_pImage->bytesPerChannel *
                          this->_pImage->channels *
                          (y * this->_pImage->width + x);

    return static_cast<int64_t>(
        this->_pImage->pixelData[pixelOffset + this->_channel]);
  }

  const FeatureTable* getFeatureTable() const noexcept {
    return this->_pFeatureTable;
  }

  int64_t getTextureCoordinateIndex() const noexcept {
    return this->_textureCoordinateIndex;
  }

private:
  const ImageCesium* _pImage;
  int32_t _channel;
  int64_t _textureCoordinateIndex;
  const FeatureTable* _pFeatureTable;
  FeatureIDTextureViewStatus _status;
};

} // namespace CesiumGltf