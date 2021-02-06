#pragma once

#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/spdlog-cesium.h"

#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace Cesium3DTilesTests
{
	/**
	 * @brief Implementation of IPrepareRendererResources that does nothing.
	 * 
	 * Except for logging.
	 */
	class NullResourcePreparer : public Cesium3DTiles::IPrepareRendererResources {
	public:
		NullResourcePreparer();

		void* prepareInLoadThread(const CesiumGltf::Model& model, const glm::dmat4& transform) override;

		void* prepareInMainThread(Cesium3DTiles::Tile& tile, void* pLoadThreadResult) override;

		void free(Cesium3DTiles::Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) noexcept override;

		void* prepareRasterInLoadThread(const CesiumGltf::ImageCesium& image) override;

		void* prepareRasterInMainThread(const Cesium3DTiles::RasterOverlayTile& rasterTile, void* pLoadThreadResult) override;

		void freeRaster(const Cesium3DTiles::RasterOverlayTile& rasterTile, void* pLoadThreadResult, void* pMainThreadResult) noexcept override;

		void attachRasterInMainThread(
			const Cesium3DTiles::Tile& tile,
			uint32_t overlayTextureCoordinateID,
			const Cesium3DTiles::RasterOverlayTile& rasterTile,
			void* pMainThreadRendererResources,
			const CesiumGeometry::Rectangle& textureCoordinateRectangle,
			const glm::dvec2& translation,
			const glm::dvec2& scale) override;

		void detachRasterInMainThread(
			const Cesium3DTiles::Tile& tile,
			uint32_t overlayTextureCoordinateID,
			const Cesium3DTiles::RasterOverlayTile& rasterTile,
			void* pMainThreadRendererResources,
			const CesiumGeometry::Rectangle& textureCoordinateRectangle
		) noexcept override;
	};
}