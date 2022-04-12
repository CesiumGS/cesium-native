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

struct TileRenderContent {
  std::optional<CesiumGltf::Model> model{};
};

struct TileExternalContent {};

struct TileEmptyContent {};

using TileContentKind =
    std::variant<TileRenderContent, TileExternalContent, TileEmptyContent>;

struct TileContent {
  TileContentKind contentKind;

  TileLoadState state{TileLoadState::Unloaded};

  TilesetContentLoader* tilesetLoader{nullptr};
};
} // namespace Cesium3DTilesSelection
