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
  InvalidChannelCount
};

class FeatureIDTextureView {
public:
  FeatureIDTextureView() noexcept
      : _pImage(nullptr),
        _textureCoordinateIndex(-1),
        _pFeatureTable(nullptr),
        _status(FeatureIDTextureViewStatus::InvalidUninitialized) {}

  FeatureIDTextureView(
      const Model& model,
      const FeatureIDTexture& featureIDTexture) noexcept
      : _pImage(nullptr),
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

    if (this->_pImage->channels != 1) {
      this->_status = FeatureIDTextureViewStatus::InvalidChannelCount;
      return;
    }

    // TODO: add more validation steps, check if image has at least one pixel.

    // TODO: once compressed texture support is merged, check that the image is
    // decompressed here.
  }

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
  uint8_t getFeatureID(double u, double v) const noexcept {
    assert(this->_status == FeatureIDTextureViewStatus::Valid);

    int64_t x = std::clamp(
        std::lround(u * this->_pImage->width),
        0L,
        (long)this->_pImage->width);
    int64_t y = std::clamp(
        std::lround(v * this->_pImage->height),
        0L,
        (long)this->_pImage->height);
    int64_t offset = y * this->_pImage->width + x;

    return static_cast<uint8_t>(this->_pImage->pixelData[offset]);
  }

  const FeatureTable* getFeatureTable() const noexcept {
    return this->_pFeatureTable;
  }

  int64_t getTextureCoordinateIndex() const noexcept {
    return this->_textureCoordinateIndex;
  }

private:
  const ImageCesium* _pImage;
  int64_t _textureCoordinateIndex;
  const FeatureTable* _pFeatureTable;
  FeatureIDTextureViewStatus _status;
};

} // namespace CesiumGltf