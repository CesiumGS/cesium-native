#pragma once

#include <Cesium3DTilesSelection/Exp_TileUserDataStorage.h>
#include <CesiumGltf/Model.h>

#include <optional>
#include <variant>
#include <functional>

namespace Cesium3DTilesSelection {
class TilesetContentLoader;
class Tile;

enum class TileLoadState {
  Failed = -2,
  FailedTemporarily = -1,
  Unloaded = 0,
  ContentLoading = 1,
  ContentLoaded = 2,
  Done = 3
};

struct TileUnknownContent {};

struct TileEmptyContent {};

struct TileExternalContent {};

struct TileRenderContent {
  std::optional<CesiumGltf::Model> model{};
};

using TileContentKind = std::variant<
    TileUnknownContent,
    TileEmptyContent,
    TileExternalContent,
    TileRenderContent>;

class TileContent {
public:
  TileContent(TilesetContentLoader* pLoader);

  TileLoadState getState() const noexcept;

  bool isExternalContent() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TileRenderContent* getRenderContent() noexcept;

  TileUserDataStorage::Handle getCustomDataHandle() const;

  TilesetContentLoader* getLoader() noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void setState(TileLoadState state) noexcept;

  void setCustomDataHandle(TileUserDataStorage::Handle handle);

  void setRenderResources(void* pRenderResources) noexcept;

  void* getRenderResources() noexcept;

  void setTileInitializerCallback(std::function<void(Tile&)> callback);

  std::function<void(Tile&)> &getTileInitializerCallback();

  TileLoadState _state;
  TileContentKind _contentKind;
  TileUserDataStorage::Handle _loaderCustomDataHandle;
  void* _pRenderResources;
  std::function<void(Tile&)> _deferredTileInitializer;
  TilesetContentLoader* _pLoader;

  friend class TilesetContentManager;
};
} // namespace Cesium3DTilesSelection
