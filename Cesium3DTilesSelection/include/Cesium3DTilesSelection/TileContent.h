#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/RasterOverlayDetails.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Model.h>

#include <variant>
#include <memory>

namespace Cesium3DTilesSelection {
struct TileUnknownContent {};

struct TileEmptyContent {};

struct TileExternalContent {};

class TileRenderContent {
public:
  TileRenderContent(CesiumGltf::Model&& model);

  const CesiumGltf::Model& getModel() const noexcept;

  CesiumGltf::Model& getModel() noexcept;

  void setModel(const CesiumGltf::Model& model);

  void setModel(CesiumGltf::Model&& model);

  const RasterOverlayDetails& getRasterOverlayDetails() const noexcept;

  RasterOverlayDetails& getRasterOverlayDetails() noexcept;

  void
  setRasterOverlayDetails(const RasterOverlayDetails& rasterOverlayDetails);

  void setRasterOverlayDetails(RasterOverlayDetails&& rasterOverlayDetails);

  const std::vector<Credit>& getCredits() const noexcept;

  std::vector<Credit>& getCredits() noexcept;

  void setCredits(std::vector<Credit>&& credits);

  void setCredits(const std::vector<Credit>& credits);

  void* getRenderResources() const noexcept;

  void setRenderResources(void* pRenderResources) noexcept;

private:
  CesiumGltf::Model _model;
  void* _pRenderResources;
  RasterOverlayDetails _rasterOverlayDetails;
  std::vector<Credit> _credits;
};

class TileContent {
  using TileContentKindImpl = std::variant<
      TileUnknownContent,
      TileEmptyContent,
      TileExternalContent,
      std::unique_ptr<TileRenderContent>>;

public:
  TileContent();

  TileContent(TileEmptyContent content);

  TileContent(TileExternalContent content);

  void setContentKind(TileUnknownContent content);

  void setContentKind(TileEmptyContent content);

  void setContentKind(TileExternalContent content);

  void setContentKind(std::unique_ptr<TileRenderContent> content);

  bool isUnknownContent() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isExternalContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TileRenderContent* getRenderContent() noexcept;

private:
  TileContentKindImpl _contentKind;
};
} // namespace Cesium3DTilesSelection
