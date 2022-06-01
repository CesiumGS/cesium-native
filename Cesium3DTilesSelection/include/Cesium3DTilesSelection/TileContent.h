#pragma once

#include <CesiumGeospatial/Projection.h>
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

  TileContent(TilesetContentLoader* pLoader, TileEmptyContent emptyContent);

  TileContent(
      TilesetContentLoader* pLoader,
      TileExternalContent externalContent);

  TileLoadState getState() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  const CesiumGeospatial::Projection* getProjection() const noexcept;

  TilesetContentLoader* getLoader() noexcept;

  void* getRenderResources() const noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void setProjection(const CesiumGeospatial::Projection& projection);

  void setState(TileLoadState state) noexcept;

  void setRenderResources(void* pRenderResources) noexcept;

  void setTileInitializerCallback(std::function<void(Tile&)> callback);

  std::function<void(Tile&)>& getTileInitializerCallback();

  TileLoadState _state;
  TileContentKind _contentKind;
  void* _pRenderResources;
  std::optional<CesiumGeospatial::Projection> _projection;
  std::function<void(Tile&)> _tileInitializer;
  TilesetContentLoader* _pLoader;

  friend class TilesetContentManager;
};
} // namespace Cesium3DTilesSelection
