#pragma once

#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/Tile.h"

#include <atomic>

namespace Cesium3DTilesSelection {
class SimplePrepareRendererResource
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  std::atomic<size_t> totalAllocation{};
  std::vector<std::byte> mockClientData{};
  bool mockShouldCacheResponseData{true};

  struct AllocationResult {
    AllocationResult(std::atomic<size_t>& allocCount_)
        : allocCount{allocCount_} {
      ++allocCount;
    }

    ~AllocationResult() noexcept { --allocCount; }

    std::atomic<size_t>& allocCount;
  };

  ~SimplePrepareRendererResource() noexcept { CHECK(totalAllocation == 0); }

  virtual CesiumAsync::Future<ClientTileLoadResult> prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TileLoadResult&& tileLoadResult,
      const glm::dmat4& /*transform*/,
      const std::any& /*rendererOptions*/) override {
    // TODO: might be less awkward to have a separate client function 
    // implemented from IPrepareRenderResources that is responsible for 
    // re-creating render content from binary client data.
    /*
    if (std::get_if<TileCachedRenderContent>(&tileLoadResult.contentKind)) {
      // TODO: mock creating render data from client data response.
      // tileLoadResult.contentKind = deserializeModel(pResponse->clientData());
      // or 
      // pRenderResources = deserializeRenderResource(pResponse->clientData());
    }*/

    // Up to the client if to update the cache, even on cache hits.
    std::vector<std::byte> clientData = this->mockClientData;
    return asyncSystem.createResolvedFuture(ClientTileLoadResult{
        std::move(tileLoadResult),
        new AllocationResult{totalAllocation},
        mockShouldCacheResponseData,
        std::move(clientData)});
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
      CesiumGltf::ImageCesium& /*image*/,
      const std::any& /*rendererOptions*/) override {
    return new AllocationResult{totalAllocation};
  }

  virtual void* prepareRasterInMainThread(
      Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult) override {
    if (pLoadThreadResult) {
      AllocationResult* loadThreadResult =
          reinterpret_cast<AllocationResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }

    return new AllocationResult{totalAllocation};
  }

  virtual void freeRaster(
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
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
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/,
      const glm::dvec2& /*translation*/,
      const glm::dvec2& /*scale*/) override {}

  virtual void detachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& /*tile*/,
      int32_t /*overlayTextureCoordinateID*/,
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* /*pMainThreadRendererResources*/) noexcept override {}
};
} // namespace Cesium3DTilesSelection
