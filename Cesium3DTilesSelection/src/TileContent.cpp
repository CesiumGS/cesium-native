#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
TileRenderContent::TileRenderContent(CesiumGltf::Model&& model)
    : _model{std::move(model)},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{},
      _credits{},
      _lodTransitionFadePercentage{0.0f} {}

const CesiumGltf::Model& TileRenderContent::getModel() const noexcept {
  return _model;
}

CesiumGltf::Model& TileRenderContent::getModel() noexcept { return _model; }

void TileRenderContent::setModel(const CesiumGltf::Model& model) {
  _model = model;
}

void TileRenderContent::setModel(CesiumGltf::Model&& model) {
  _model = std::move(model);
}

GltfModifier::State TileRenderContent::getGltfModifierState() const noexcept {
  return _modifierState;
}

void TileRenderContent::setGltfModifierState(
    GltfModifier::State modifierState) noexcept {
  _modifierState = modifierState;
}

const std::optional<CesiumGltf::Model>&
TileRenderContent::getModifiedModel() const noexcept {
  return _modifiedModel;
}

void TileRenderContent::setModifiedModelAndRenderResources(
    CesiumGltf::Model&& modifiedModel,
    void* pModifiedRenderResources) noexcept {
  _modifiedModel = std::move(modifiedModel);
  _pModifiedRenderResources = pModifiedRenderResources;
}

void* TileRenderContent::getModifiedRenderResources() const noexcept {
  return _pModifiedRenderResources;
}

void TileRenderContent::resetModifiedRenderResources() noexcept {
  _pModifiedRenderResources = nullptr;
}

void TileRenderContent::replaceWithModifiedModel() noexcept {
  CESIUM_ASSERT(this->_modifiedModel);
  if (this->_modifiedModel) {
    this->_model = std::move(*this->_modifiedModel);
    // reset after move because tested in Tile::needsWorkerThreadLoading:
    this->_modifiedModel.reset();
    this->_pRenderResources = this->_pModifiedRenderResources;
    this->_pModifiedRenderResources = nullptr;
  }
}

const RasterOverlayDetails&
TileRenderContent::getRasterOverlayDetails() const noexcept {
  return this->_rasterOverlayDetails;
}

RasterOverlayDetails& TileRenderContent::getRasterOverlayDetails() noexcept {
  return this->_rasterOverlayDetails;
}

void TileRenderContent::setRasterOverlayDetails(
    const RasterOverlayDetails& rasterOverlayDetails) {
  this->_rasterOverlayDetails = rasterOverlayDetails;
}

void TileRenderContent::setRasterOverlayDetails(
    RasterOverlayDetails&& rasterOverlayDetails) {
  this->_rasterOverlayDetails = std::move(rasterOverlayDetails);
}

const std::vector<Credit>& TileRenderContent::getCredits() const noexcept {
  return this->_credits;
}

std::vector<Credit>& TileRenderContent::getCredits() noexcept {
  return this->_credits;
}

void TileRenderContent::setCredits(std::vector<Credit>&& credits) {
  this->_credits = std::move(credits);
}

void TileRenderContent::setCredits(const std::vector<Credit>& credits) {
  this->_credits = credits;
}

void* TileRenderContent::getRenderResources() const noexcept {
  return this->_pRenderResources;
}

void TileRenderContent::setRenderResources(void* pRenderResources) noexcept {
  this->_pRenderResources = pRenderResources;
}

float TileRenderContent::getLodTransitionFadePercentage() const noexcept {
  return _lodTransitionFadePercentage;
}

void TileRenderContent::setLodTransitionFadePercentage(
    float percentage) noexcept {
  this->_lodTransitionFadePercentage = percentage;
}

TileContent::TileContent() : _contentKind{TileUnknownContent{}} {}

TileContent::TileContent(TileEmptyContent content) : _contentKind{content} {}

TileContent::TileContent(std::unique_ptr<TileExternalContent>&& content)
    : _contentKind{std::move(content)} {}

void TileContent::setContentKind(TileUnknownContent content) {
  _contentKind = content;
}

void TileContent::setContentKind(TileEmptyContent content) {
  _contentKind = content;
}

void TileContent::setContentKind(
    std::unique_ptr<TileExternalContent>&& content) {
  _contentKind = std::move(content);
}

void TileContent::setContentKind(std::unique_ptr<TileRenderContent>&& content) {
  _contentKind = std::move(content);
}

bool TileContent::isUnknownContent() const noexcept {
  return std::holds_alternative<TileUnknownContent>(this->_contentKind);
}

bool TileContent::isEmptyContent() const noexcept {
  return std::holds_alternative<TileEmptyContent>(this->_contentKind);
}

bool TileContent::isExternalContent() const noexcept {
  return std::holds_alternative<std::unique_ptr<TileExternalContent>>(
      this->_contentKind);
}

bool TileContent::isRenderContent() const noexcept {
  return std::holds_alternative<std::unique_ptr<TileRenderContent>>(
      this->_contentKind);
}

const TileRenderContent* TileContent::getRenderContent() const noexcept {
  const std::unique_ptr<TileRenderContent>* pRenderContent =
      std::get_if<std::unique_ptr<TileRenderContent>>(&this->_contentKind);
  if (pRenderContent) {
    return pRenderContent->get();
  }

  return nullptr;
}

TileRenderContent* TileContent::getRenderContent() noexcept {
  std::unique_ptr<TileRenderContent>* pRenderContent =
      std::get_if<std::unique_ptr<TileRenderContent>>(&this->_contentKind);
  if (pRenderContent) {
    return pRenderContent->get();
  }

  return nullptr;
}

const TileExternalContent* TileContent::getExternalContent() const noexcept {
  const std::unique_ptr<TileExternalContent>* pExternalContent =
      std::get_if<std::unique_ptr<TileExternalContent>>(&this->_contentKind);
  if (pExternalContent) {
    return pExternalContent->get();
  }

  return nullptr;
}

TileExternalContent* TileContent::getExternalContent() noexcept {
  std::unique_ptr<TileExternalContent>* pExternalContent =
      std::get_if<std::unique_ptr<TileExternalContent>>(&this->_contentKind);
  if (pExternalContent) {
    return pExternalContent->get();
  }

  return nullptr;
}

} // namespace Cesium3DTilesSelection
