
#include "NullResourcePreparer.h"
#include "Cesium3DTilesTestUtils.h"

#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"

#include "Cesium3DTiles/TileContentLoadResult.h"
#include "CesiumGltf/Model.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace Cesium3DTilesTests
{
	NullResourcePreparer::NullResourcePreparer() {
		// Nothingto do here
	}

	void* NullResourcePreparer::prepareInLoadThread(const CesiumGltf::Model& /*model*/, const glm::dmat4& /*transform*/) {
		SPDLOG_TRACE("Called NullResourcePreparer::prepareInLoadThread");
		return nullptr;
	}

	void* NullResourcePreparer::prepareInMainThread(Cesium3DTiles::Tile& /*tile*/, void* /*pLoadThreadResult*/) {
		SPDLOG_TRACE("Called NullResourcePreparer::prepareInMainThread");
		return nullptr;
	}

	void NullResourcePreparer::free(Cesium3DTiles::Tile& /*tile*/, void* /*pLoadThreadResult*/, void* /*pMainThreadResult*/) noexcept {
		SPDLOG_TRACE("Called NullResourcePreparer::free");
	}

	void* NullResourcePreparer::prepareRasterInLoadThread(const CesiumGltf::ImageCesium& /*image*/) {
		SPDLOG_TRACE("Called NullResourcePreparer::prepareRasterInLoadThread");
		return nullptr;
	}

	void* NullResourcePreparer::prepareRasterInMainThread(const Cesium3DTiles::RasterOverlayTile& /*rasterTile*/, void* /*pLoadThreadResult*/) {
		SPDLOG_TRACE("Called NullResourcePreparer::prepareRasterInMainThread");
		return nullptr;
	}

	void NullResourcePreparer::freeRaster(const Cesium3DTiles::RasterOverlayTile& /*rasterTile*/, void* /*pLoadThreadResult*/, void* /*pMainThreadResult*/) noexcept {
		SPDLOG_TRACE("Called NullResourcePreparer::freeRaster");
	}

	void NullResourcePreparer::attachRasterInMainThread(
		const Cesium3DTiles::Tile& /*tile*/,
		uint32_t /*overlayTextureCoordinateID*/,
		const Cesium3DTiles::RasterOverlayTile& /*rasterTile*/,
		void* /*pMainThreadRendererResources*/,
		const CesiumGeometry::Rectangle& /*textureCoordinateRectangle*/,
		const glm::dvec2& /*translation*/,
		const glm::dvec2& /*scale*/
	) {
		SPDLOG_TRACE("Called NullResourcePreparer::attachRasterInMainThread");
	}

	void NullResourcePreparer::detachRasterInMainThread(
		const Cesium3DTiles::Tile& /*tile*/,
		uint32_t /*overlayTextureCoordinateID*/,
		const Cesium3DTiles::RasterOverlayTile& /*rasterTile*/,
		void* /*pMainThreadRendererResources*/,
		const CesiumGeometry::Rectangle& /*textureCoordinateRectangle*/
	) noexcept {
		SPDLOG_TRACE("Called NullResourcePreparer::detachRasterInMainThread");
	}

}

