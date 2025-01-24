#pragma once

#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>

#include <doctest/doctest.h>

#include <atomic>

namespace Cesium3DTilesSelection {
class SimplePrepareRendererResource
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  std::atomic<size_t> totalAllocation{};

  struct AllocationResult {
    AllocationResult(std::atomic<size_t>& allocCount_)
        : allocCount{allocCount_} {
      ++allocCount;
    }

    ~AllocationResult() noexcept { --allocCount; }

    std::atomic<size_t>& allocCount;
  };

  ~SimplePrepareRendererResource() noexcept { CHECK(totalAllocation == 0); }

  virtual CesiumAsync::Future<TileLoadResultAndRenderResources>
  prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TileLoadResult&& tileLoadResult,
      const glm::dmat4& /*transform*/,
      const std::any& /*rendererOptions*/) override {
    return asyncSystem.createResolvedFuture(TileLoadResultAndRenderResources{
        std::move(tileLoadResult),
        new AllocationResult{totalAllocation}});
  }

  virtual void* prepareInMainThread(
      Cesium3DTilesSelection::Tile& /*tile*/,
      void* pLoadThreadResult) override {
    if (pLoadThreadResult) {
      AllocationResult* loadThreadResult =
          reinterpret_cast<AllocationResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }

    return new AllocationResult{totalAllocation};
  }

  virtual void free(
      Cesium3DTilesSelection::Tile& /*tile*/,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pMainThreadResult) {
      AllocationResult* mainThreadResult =
          reinterpret_cast<AllocationResult*>(pMainThreadResult);
      delete mainThreadResult;
    }

    if (pLoadThreadResult) {
      AllocationResult* loadThreadResult =
          reinterpret_cast<AllocationResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }
  }

  virtual void* prepareRasterInLoadThread(
      CesiumGltf::ImageAsset& /*image*/,
      const std::any& /*rendererOptions*/) override {
    return new AllocationResult{totalAllocation};
  }

  virtual void* prepareRasterInMainThread(
      CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult) override {
    if (pLoadThreadResult) {
      AllocationResult* loadThreadResult =
          reinterpret_cast<AllocationResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }

    return new AllocationResult{totalAllocation};
  }

  virtual void freeRaster(
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pMainThreadResult) {
      AllocationResult* mainThreadResult =
          reinterpret_cast<AllocationResult*>(pMainThreadResult);
      delete mainThreadResult;
    }

    if (pLoadThreadResult) {
      AllocationResult* loadThreadResult =
          reinterpret_cast<AllocationResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }
  }

  virtual void attachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& /*tile*/,
      int32_t /*overlayTextureCoordinateID*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/,
      const glm::dvec2& /*translation*/,
      const glm::dvec2& /*scale*/) override {}

  virtual void detachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& /*tile*/,
      int32_t /*overlayTextureCoordinateID*/,
      const CesiumRasterOverlays::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/) noexcept override {}
};
} // namespace Cesium3DTilesSelection
