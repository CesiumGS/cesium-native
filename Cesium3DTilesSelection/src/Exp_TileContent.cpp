#include <Cesium3DTilesSelection/Exp_TileContent.h>

namespace Cesium3DTilesSelection {
TileContent::TileContent(TilesetContentLoader* pLoader)
    : _state{TileLoadState::Unloaded},
      _contentKind{TileUnknownContent{}},
      _pRenderResources{nullptr},
      _deferredTileInitializer {},
      _pLoader{pLoader} {}

TileContent::TileContent(TilesetContentLoader* pLoader, TileEmptyContent emptyContent)
    : _state{TileLoadState::ContentLoaded},
      _contentKind{emptyContent},
      _pRenderResources{nullptr},
      _deferredTileInitializer {},
      _pLoader{pLoader} {}

TileContent::TileContent(TilesetContentLoader* pLoader, TileExternalContent externalContent)
    : _state{TileLoadState::ContentLoaded},
      _contentKind{externalContent},
      _pRenderResources{nullptr},
      _deferredTileInitializer {},
      _pLoader{pLoader} {}

TileLoadState TileContent::getState() const noexcept { return _state; }

bool TileContent::isEmptyContent() const noexcept {
  return std::holds_alternative<TileEmptyContent>(_contentKind);
}

bool TileContent::isExternalContent() const noexcept {
  return std::holds_alternative<TileExternalContent>(_contentKind);
}

bool TileContent::isRenderContent() const noexcept {
  return std::holds_alternative<TileRenderContent>(_contentKind);
}

const TileRenderContent* TileContent::getRenderContent() const noexcept {
  return std::get_if<TileRenderContent>(&_contentKind);
}

TilesetContentLoader* TileContent::getLoader() noexcept { return _pLoader; }

void TileContent::setContentKind(TileContentKind&& contentKind) {
  _contentKind = std::move(contentKind);
}

void TileContent::setContentKind(const TileContentKind& contentKind) {
  _contentKind = contentKind;
}

void TileContent::setState(TileLoadState state) noexcept { _state = state; }

void TileContent::setRenderResources(void* pRenderResources) noexcept {
  _pRenderResources = pRenderResources;
}

void* TileContent::getRenderResources() const noexcept {
  return _pRenderResources;
}

void TileContent::setTileInitializerCallback(
    std::function<void(Tile&)> callback) {
  _deferredTileInitializer = std::move(callback);
}

std::function<void(Tile&)>& TileContent::getTileInitializerCallback() {
  return _deferredTileInitializer;
}
} // namespace Cesium3DTilesSelection
