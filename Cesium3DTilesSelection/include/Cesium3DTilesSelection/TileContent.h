#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <CesiumGltf/Model.h>
#include <optional>

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

struct TileContent {
  std::optional<CesiumGltf::Model> model{};

  std::vector<Cesium3DTilesSelection::Credit> credits{};

  TileLoadState state{TileLoadState::Unloaded};

  TilesetContentLoader* loader{nullptr};
};
} // namespace Cesium3DTilesSelection
