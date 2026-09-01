#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/Texture.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumMetadata/PropertyTexturePropertyView.h>
#include <CesiumMetadata/PropertyTextureView.h>
#include <CesiumMetadata/PropertyView.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumMetadata {
PropertyTextureView::PropertyTextureView(
    const CesiumGltf::Model& model,
    const CesiumGltf::PropertyTexture& propertyTexture) noexcept
    : _pModel(&model),
      _pPropertyTexture(&propertyTexture),
      _pClass(nullptr),
      _pEnumDefinitions{},
      _status() {
  const auto* pMetadata =
      model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();

  if (!pMetadata) {
    this->_status = PropertyTextureViewStatus::ErrorMissingMetadataExtension;
    return;
  }

  if (!pMetadata->schema) {
    this->_status = PropertyTextureViewStatus::ErrorMissingSchema;
    return;
  }

  const auto& classIt =
      pMetadata->schema->classes.find(propertyTexture.classProperty);
  if (classIt == pMetadata->schema->classes.end()) {
    this->_status = PropertyTextureViewStatus::ErrorClassNotFound;
    return;
  }

  this->_pClass = &classIt->second;
  this->_pEnumDefinitions = &pMetadata->schema->enums;
}

const CesiumGltf::ClassProperty*
PropertyTextureView::getClassProperty(const std::string& propertyId) const {
  if (_status != PropertyTextureViewStatus::Valid) {
    return nullptr;
  }

  auto propertyIter = _pClass->properties.find(propertyId);
  if (propertyIter == _pClass->properties.end()) {
    return nullptr;
  }

  return &propertyIter->second;
}

PropertyViewStatusType PropertyTextureView::getTextureSafe(
    const int32_t textureIndex,
    int32_t& samplerIndex,
    int32_t& imageIndex) const noexcept {
  const CesiumGltf::Texture* pTexture =
      this->_pModel->getSafe(&this->_pModel->textures, textureIndex);
  if (!pTexture) {
    return PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
  }

  samplerIndex = pTexture->sampler;
  imageIndex = pTexture->source;

  return PropertyTexturePropertyViewStatus::Valid;
}

PropertyViewStatusType
PropertyTextureView::checkSampler(const int32_t samplerIndex) const noexcept {
  // TODO: check if sampler filter values are supported
  return this->_pModel->getSafe(&this->_pModel->samplers, samplerIndex)
             ? PropertyTexturePropertyViewStatus::Valid
             : PropertyTexturePropertyViewStatus::ErrorInvalidSampler;
}

PropertyViewStatusType
PropertyTextureView::checkImage(const int32_t imageIndex) const noexcept {
  const CesiumGltf::Image* pImage =
      this->_pModel->getSafe(&this->_pModel->images, imageIndex);

  if (!pImage) {
    return PropertyTexturePropertyViewStatus::ErrorInvalidImage;
  }

  const CesiumUtility::IntrusivePointer<CesiumImage::ImageAsset>& pImageAsset =
      pImage->pAsset;

  if (!pImageAsset || pImageAsset->width < 1 || pImageAsset->height < 1) {
    return PropertyTexturePropertyViewStatus::ErrorEmptyImage;
  }

  if (pImageAsset->bytesPerChannel > 1) {
    return PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel;
  }

  return PropertyTexturePropertyViewStatus::Valid;
}

PropertyViewStatusType PropertyTextureView::checkChannels(
    const std::vector<int64_t>& channels,
    const CesiumImage::ImageAsset& image) const noexcept {
  if (channels.size() <= 0 || channels.size() > 4) {
    return PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
  }

  int64_t imageChannelCount = int64_t(image.channels);
  for (int64_t channel : channels) {
    if (channel < 0 || channel >= imageChannelCount) {
      return PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
    }
  }

  return PropertyTexturePropertyViewStatus::Valid;
}

} // namespace CesiumMetadata
