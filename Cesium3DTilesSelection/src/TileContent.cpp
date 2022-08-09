#include <Cesium3DTilesSelection/TileContent.h>

namespace Cesium3DTilesSelection {
TileContent::TileContent()
    : _contentKind{TileUnknownContent{}},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{},
      _tileInitializer{} {}

TileContent::TileContent(TileEmptyContent emptyContent)
    : _contentKind{emptyContent},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{},
      _tileInitializer{} {}

TileContent::TileContent(TileExternalContent externalContent)
    : _contentKind{externalContent},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{},
      _tileInitializer{} {}

void TileContent::setContentKind(TileContentKind&& contentKind) {
  this->_contentKind = std::move(contentKind);
}

void TileContent::setContentKind(const TileContentKind& contentKind) {
  this->_contentKind = contentKind;
}

bool TileContent::isEmptyContent() const noexcept {
  return std::holds_alternative<TileEmptyContent>(this->_contentKind);
}

bool TileContent::isExternalContent() const noexcept {
  return std::holds_alternative<TileExternalContent>(this->_contentKind);
}

bool TileContent::isRenderContent() const noexcept {
  return std::holds_alternative<TileRenderContent>(this->_contentKind);
}

const TileRenderContent* TileContent::getRenderContent() const noexcept {
  return std::get_if<TileRenderContent>(&this->_contentKind);
}

TileRenderContent* TileContent::getRenderContent() noexcept {
  return std::get_if<TileRenderContent>(&this->_contentKind);
}

const RasterOverlayDetails&
TileContent::getRasterOverlayDetails() const noexcept {
  return this->_rasterOverlayDetails;
}

RasterOverlayDetails& TileContent::getRasterOverlayDetails() noexcept {
  return this->_rasterOverlayDetails;
}

void TileContent::setRasterOverlayDetails(
    const RasterOverlayDetails& rasterOverlayDetails) {
  this->_rasterOverlayDetails = rasterOverlayDetails;
}

void TileContent::setRasterOverlayDetails(
    RasterOverlayDetails&& rasterOverlayDetails) {
  this->_rasterOverlayDetails = std::move(rasterOverlayDetails);
}

const std::vector<Credit>& TileContent::getCredits() const noexcept {
  return this->_credits;
}

std::vector<Credit>& TileContent::getCredits() noexcept {
  return this->_credits;
}

void TileContent::setCredits(std::vector<Credit>&& credits) {
  this->_credits = std::move(credits);
}

void TileContent::setCredits(const std::vector<Credit>& credits) {
  this->_credits = credits;
}

const std::function<void(Tile&)>&
TileContent::getTileInitializerCallback() const noexcept {
  return this->_tileInitializer;
}

void TileContent::setTileInitializerCallback(
    std::function<void(Tile&)> callback) {
  this->_tileInitializer = std::move(callback);
}

void* TileContent::getRenderResources() const noexcept {
  return this->_pRenderResources;
}

void TileContent::setRenderResources(void* pRenderResources) noexcept {
  this->_pRenderResources = pRenderResources;
}
} // namespace Cesium3DTilesSelection
