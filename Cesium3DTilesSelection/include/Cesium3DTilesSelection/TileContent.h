#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/RasterOverlayDetails.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Model.h>

#include <functional>
#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {
class Tile;

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
  TileContent();

  TileContent(TileEmptyContent emptyContent);

  TileContent(TileExternalContent externalContent);

  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  bool isEmptyContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TileRenderContent* getRenderContent() noexcept;

  const RasterOverlayDetails& getRasterOverlayDetails() const noexcept;

  RasterOverlayDetails& getRasterOverlayDetails() noexcept;

  void
  setRasterOverlayDetails(const RasterOverlayDetails& rasterOverlayDetails);

  void setRasterOverlayDetails(RasterOverlayDetails&& rasterOverlayDetails);

  const std::vector<Credit>& getCredits() const noexcept;

  std::vector<Credit>& getCredits() noexcept;

  void setCredits(std::vector<Credit>&& credits);

  void setCredits(const std::vector<Credit>& credits);

  const std::function<void(Tile&)>& getTileInitializerCallback() const noexcept;

  void setTileInitializerCallback(std::function<void(Tile&)> callback);

  void* getRenderResources() const noexcept;

  void setRenderResources(void* pRenderResources) noexcept;

private:
  TileContentKind _contentKind;
  void* _pRenderResources;
  RasterOverlayDetails _rasterOverlayDetails;
  std::function<void(Tile&)> _tileInitializer;
  std::vector<Credit> _credits;
};
} // namespace Cesium3DTilesSelection
