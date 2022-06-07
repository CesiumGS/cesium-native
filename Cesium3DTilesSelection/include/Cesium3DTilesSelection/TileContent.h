#pragma once

#include <CesiumGltf/Model.h>

#include <functional>
#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {
class TilesetContentLoader;
class Tile;

enum class TileLoadState {
  FailedTemporarily = -1,
  Unloaded = 0,
  ContentLoading = 1,
  ContentLoaded = 2,
  Done = 3,
  Failed = 4,
};

struct TileUnknownContent {};

struct TileEmptyContent {};

struct TileExternalContent {
  std::function<void(Tile&)> createSubtree;
};

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

  TileContent(TilesetContentLoader* pLoader, TileEmptyContent emptyContent);

  TileContent(
      TilesetContentLoader* pLoader,
      TileExternalContent externalContent);

  TileLoadState getState() const noexcept;

  bool shouldContentContinueUpdated() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileExternalContent* getExternalContent() const noexcept;

  TileExternalContent* getExternalContent() noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TilesetContentLoader* getLoader() noexcept;

  void* getRenderResources() const noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void setState(TileLoadState state) noexcept;

  void setRenderResources(void* pRenderResources) noexcept;

  void
  setContentShouldContinueUpdated(bool shouldContentContinueUpdated) noexcept;

  TileLoadState _state;
  TileContentKind _contentKind;
  void* _pRenderResources;
  TilesetContentLoader* _pLoader;
  bool _shouldContentContinueUpdated;

  friend class TilesetContentManager;
};
} // namespace Cesium3DTilesSelection
