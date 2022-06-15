#include <Cesium3DTilesSelection/TileContent.h>

namespace Cesium3DTilesSelection {
TileContent::TileContent(TilesetContentLoader* pLoader)
    : _state{TileLoadState::Unloaded},
      _contentKind{TileUnknownContent{}},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{std::nullopt},
      _tileInitializer{},
      _pLoader{pLoader},
      _shouldContentContinueUpdated{true} {}

TileContent::TileContent(
    TilesetContentLoader* pLoader,
    TileEmptyContent emptyContent)
    : _state{TileLoadState::ContentLoaded},
      _contentKind{emptyContent},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{std::nullopt},
      _tileInitializer{},
      _pLoader{pLoader},
      _shouldContentContinueUpdated{true} {}

TileContent::TileContent(
    TilesetContentLoader* pLoader,
    TileExternalContent externalContent)
    : _state{TileLoadState::ContentLoaded},
      _contentKind{externalContent},
      _pRenderResources{nullptr},
      _rasterOverlayDetails{std::nullopt},
      _tileInitializer{},
      _pLoader{pLoader},
      _shouldContentContinueUpdated{true} {}

TileLoadState TileContent::getState() const noexcept { return _state; }

bool TileContent::shouldContentContinueUpdated() const noexcept {
  return _shouldContentContinueUpdated;
}

bool TileContent::isEmptyContent() const noexcept {
  return std::holds_alternative<TileEmptyContent>(_contentKind);
}

bool TileContent::isExternalContent() const noexcept {
  return std::holds_alternative<TileExternalContent>(_contentKind);
}

bool TileContent::isRenderContent() const noexcept {
  return std::holds_alternative<TileRenderContent>(_contentKind);
}

const TileExternalContent* TileContent::getExternalContent() const noexcept {
  return std::get_if<TileExternalContent>(&_contentKind);
}

TileExternalContent* TileContent::getExternalContent() noexcept {
  return std::get_if<TileExternalContent>(&_contentKind);
}

const TileRenderContent* TileContent::getRenderContent() const noexcept {
  return std::get_if<TileRenderContent>(&_contentKind);
}

const RasterOverlayDetails*
TileContent::getRasterOverlayDetails() const noexcept {
  if (_rasterOverlayDetails) {
    return &*_rasterOverlayDetails;
  }

  return nullptr;
}

RasterOverlayDetails* TileContent::getRasterOverlayDetails() noexcept {
  if (_rasterOverlayDetails) {
    return &*_rasterOverlayDetails;
  }

  return nullptr;
}

TilesetContentLoader* TileContent::getLoader() noexcept { return _pLoader; }

void TileContent::setContentKind(TileContentKind&& contentKind) {
  _contentKind = std::move(contentKind);
}

void TileContent::setContentKind(const TileContentKind& contentKind) {
  _contentKind = contentKind;
}

void TileContent::setRasterOverlayDetails(
    const RasterOverlayDetails& rasterOverlayDetails) {
  _rasterOverlayDetails = rasterOverlayDetails;
}

void TileContent::setRasterOverlayDetails(
    RasterOverlayDetails&& rasterOverlayDetails) {
  _rasterOverlayDetails = std::move(rasterOverlayDetails);
}

void TileContent::setState(TileLoadState state) noexcept { _state = state; }

void TileContent::setRenderResources(void* pRenderResources) noexcept {
  _pRenderResources = pRenderResources;
}

void TileContent::setTileInitializerCallback(
    std::function<void(Tile&)> callback) {
  _tileInitializer = std::move(callback);
}

std::function<void(Tile&)>& TileContent::getTileInitializerCallback() {
  return _tileInitializer;
}

void TileContent::setContentShouldContinueUpdated(
    bool shouldContentContinueUpdated) noexcept {
  _shouldContentContinueUpdated = shouldContentContinueUpdated;
}

void* TileContent::getRenderResources() const noexcept {
  return _pRenderResources;
}
} // namespace Cesium3DTilesSelection
