#pragma once

#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/Tile.h"

class SimplePrepareRendererResource
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  struct LoadThreadResult {};

  struct MainThreadResult {};

  struct LoadThreadRasterResult {};

  struct MainThreadRasterResult {};

  virtual void* prepareInLoadThread(
      const CesiumGltf::Model& /*model*/,
      const glm::dmat4& /*transform*/) override {
    return new LoadThreadResult{};
  }

  virtual void* prepareInMainThread(
      Cesium3DTilesSelection::Tile& /*tile*/,
      void* /*pLoadThreadResult*/) override {
    return new MainThreadResult{};
  }

  virtual void free(
      Cesium3DTilesSelection::Tile& /*tile*/,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pMainThreadResult) {
      MainThreadResult* mainThreadResult =
          reinterpret_cast<MainThreadResult*>(pMainThreadResult);
      delete mainThreadResult;
    }

    if (pLoadThreadResult) {
      LoadThreadResult* loadThreadResult =
          reinterpret_cast<LoadThreadResult*>(pLoadThreadResult);
      delete loadThreadResult;
    }
  }

  virtual void* prepareRasterInLoadThread(
      const CesiumGltf::ImageCesium& /*image*/,
      void* /*pRendererOptions*/) override {
    return new LoadThreadRasterResult{};
  }

  virtual void* prepareRasterInMainThread(
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* /*pLoadThreadResult*/) override {
    return new MainThreadRasterResult{};
  }

  virtual void freeRaster(
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pMainThreadResult) {
      MainThreadRasterResult* mainThreadResult =
          reinterpret_cast<MainThreadRasterResult*>(pMainThreadResult);
      delete mainThreadResult;
    }

    if (pLoadThreadResult) {
      LoadThreadRasterResult* loadThreadResult =
          reinterpret_cast<LoadThreadRasterResult*>(pLoadThreadResult);
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
