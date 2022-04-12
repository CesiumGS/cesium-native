#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <CesiumGltf/Model.h>

#include <any>
#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {
class TilesetContentLoader;

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

  TilesetContentLoader* getLoader() noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void setState(TileLoadState state) noexcept;

  TileContentKind _contentKind;
  TileLoadState _state;
  TilesetContentLoader* _pLoader;

  friend class TilesetContentLoader;
};
} // namespace Cesium3DTilesSelection
