#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/SamplerUtility.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/TextureInfo.h>
#include <CesiumGltf/TextureView.h>
#include <CesiumUtility/Assert.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace CesiumGltf {

TextureView::TextureView() noexcept
    : _textureViewStatus(TextureViewStatus::ErrorUninitialized),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(-1),
      _applyTextureTransform(false),
      _textureTransform(std::nullopt),
      _pImageCopy(nullptr) {}

TextureView::TextureView(
    const Model& model,
    const TextureInfo& textureInfo,
    const TextureViewOptions& options) noexcept
    : _textureViewStatus(TextureViewStatus::ErrorUninitialized),
      _pSampler(nullptr),
      _pImage(nullptr),
      _texCoordSetIndex(textureInfo.texCoord),
      _applyTextureTransform(options.applyKhrTextureTransformExtension),
      _textureTransform(std::nullopt),
      _pImageCopy(nullptr) {
  int32_t textureIndex = textureInfo.index;
  if (textureIndex < 0 ||
      static_cast<size_t>(textureIndex) >= model.textures.size()) {
    this->_textureViewStatus = TextureViewStatus::ErrorInvalidTexture;
    return;
  }

  const Texture& texture = model.textures[static_cast<size_t>(textureIndex)];
  if (texture.source < 0 ||
      static_cast<size_t>(texture.source) >= model.images.size()) {
    this->_textureViewStatus = TextureViewStatus::ErrorInvalidImage;
    return;
  }

  this->_pImage = model.images[static_cast<size_t>(texture.source)].pAsset;
  if (!this->_pImage || this->_pImage->width < 1 || this->_pImage->height < 1) {
    this->_textureViewStatus = TextureViewStatus::ErrorEmptyImage;
    return;
  }

  if (texture.sampler < 0 ||
      static_cast<size_t>(texture.sampler) >= model.samplers.size()) {
    this->_textureViewStatus = TextureViewStatus::ErrorInvalidSampler;
    return;
  }

  this->_pSampler = &model.samplers[static_cast<size_t>(texture.sampler)];

  // TODO: once compressed texture support is merged, check that the image is
  // decompressed here.

  if (this->_pImage->bytesPerChannel > 1) {
    this->_textureViewStatus = TextureViewStatus::ErrorInvalidBytesPerChannel;
    return;
  }

  const ExtensionKhrTextureTransform* pTextureTransform =
      textureInfo.getExtension<ExtensionKhrTextureTransform>();

  if (pTextureTransform) {
    this->_textureTransform = KhrTextureTransform(*pTextureTransform);
  }

  if (options.makeImageCopy) {
    this->_pImageCopy =
        this->_pImage ? new ImageAsset(*this->_pImage) : nullptr;
  }

  this->_textureViewStatus = TextureViewStatus::Valid;
}

TextureView::TextureView(
    const Sampler& sampler,
    const ImageAsset& image,
    int64_t textureCoordinateSetIndex,
    const ExtensionKhrTextureTransform* pKhrTextureTransformExtension,
    const TextureViewOptions& options) noexcept
    : _textureViewStatus(TextureViewStatus::ErrorUninitialized),
      _pSampler(&sampler),
      _pImage(new ImageAsset(image)),
      _texCoordSetIndex(textureCoordinateSetIndex),
      _applyTextureTransform(options.applyKhrTextureTransformExtension),
      _textureTransform(std::nullopt),
      _pImageCopy(nullptr) {
  // TODO: once compressed texture support is merged, check that the image is
  // decompressed here.

  if (this->_pImage->bytesPerChannel > 1) {
    this->_textureViewStatus = TextureViewStatus::ErrorInvalidBytesPerChannel;
    return;
  }

  if (pKhrTextureTransformExtension) {
    this->_textureTransform =
        KhrTextureTransform(*pKhrTextureTransformExtension);
  }

  if (options.makeImageCopy) {
    this->_pImageCopy =
        this->_pImage ? new ImageAsset(*this->_pImage) : nullptr;
  }

  this->_textureViewStatus = TextureViewStatus::Valid;
}

std::vector<uint8_t> TextureView::sampleNearestPixel(
    double u,
    double v,
    const std::vector<int64_t>& channels) const noexcept {
  CESIUM_ASSERT(this->_textureViewStatus == TextureViewStatus::Valid);
  std::vector<uint8_t> result(channels.size());

  if (channels.size() == 0) {
    return result;
  }

  if (this->_applyTextureTransform && this->_textureTransform) {
    glm::dvec2 transformedUvs = this->_textureTransform->applyTransform(u, v);
    u = transformedUvs.x;
    v = transformedUvs.y;
  }

  u = applySamplerWrapS(u, this->_pSampler->wrapS);
  v = applySamplerWrapT(v, this->_pSampler->wrapT);

  const ImageAsset& image =
      this->_pImageCopy != nullptr ? *this->_pImageCopy : *this->_pImage;

  // For nearest filtering, std::floor is used instead of std::round.
  // This is because filtering is supposed to consider the pixel centers. But
  // memory access here acts as sampling the beginning of the pixel. Example:
  // 0.4 * 2 = 0.8. In a 2x1 pixel image, that should be closer to the left
  // pixel's center. But it will round to 1.0 which corresponds to the right
  // pixel. So the right pixel has a bigger range than the left one, which is
  // incorrect.
  double xCoord = std::floor(u * image.width);
  double yCoord = std::floor(v * image.height);

  // Clamp to ensure no out-of-bounds data access
  int64_t x = glm::clamp(
      static_cast<int64_t>(xCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(image.width) - 1);
  int64_t y = glm::clamp(
      static_cast<int64_t>(yCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(image.height) - 1);

  int64_t pixelIndex =
      static_cast<int64_t>(image.bytesPerChannel * image.channels) *
      (y * image.width + x);

  // TODO: Currently stb only outputs uint8 pixel types. If that
  // changes this should account for additional pixel byte sizes.
  const uint8_t* pValue =
      reinterpret_cast<const uint8_t*>(image.pixelData.data() + pixelIndex);
  for (size_t i = 0; i < result.size(); i++) {
    result[i] = *(pValue + channels[i]);
  }

  return result;
}
} // namespace CesiumGltf
