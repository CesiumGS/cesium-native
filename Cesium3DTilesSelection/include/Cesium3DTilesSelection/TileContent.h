#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/RasterOverlayDetails.h>
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
  CreatingResources = 3,
  Done = 4,
  Failed = 5,
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

  bool shouldContentContinueUpdated() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileExternalContent* getExternalContent() const noexcept;

  TileExternalContent* getExternalContent() noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TileRenderContent* getRenderContent() noexcept;

  const RasterOverlayDetails& getRasterOverlayDetails() const noexcept;

  RasterOverlayDetails& getRasterOverlayDetails() noexcept;

  const std::vector<Credit>& getCredits() const noexcept;

  std::vector<Credit>& getCredits() noexcept;

  TilesetContentLoader* getLoader() noexcept;

  void* getRenderResources() const noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void
  setRasterOverlayDetails(const RasterOverlayDetails& rasterOverlayDetails);

  void setRasterOverlayDetails(RasterOverlayDetails&& rasterOverlayDetails);

  void setState(TileLoadState state) noexcept;

  void setRenderResources(void* pRenderResources) noexcept;

  void setTileInitializerCallback(std::function<void(Tile&)> callback);

  std::function<void(Tile&)>& getTileInitializerCallback();

  void
  setContentShouldContinueUpdated(bool shouldContentContinueUpdated) noexcept;

  void setCredits(std::vector<Credit>&& credits);

  TileLoadState _state;
  TileContentKind _contentKind;
  void* _pRenderResources;
  RasterOverlayDetails _rasterOverlayDetails;
  std::function<void(Tile&)> _tileInitializer;
  bool _shouldContentContinueUpdated;
  std::vector<Credit> _credits;
  TilesetContentLoader* _pLoader;

  friend class TilesetContentManager;
};
} // namespace Cesium3DTilesSelection
