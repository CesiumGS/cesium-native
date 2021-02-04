#include "catch2/catch.hpp"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"

using namespace CesiumAsync;
using namespace Cesium3DTiles;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

class MockAssetAccessor : public IAssetAccessor {

};

class MockTaskProcessor : public ITaskProcessor {
public:
	virtual void startTask(std::function<void()> f) override { f(); }
};

class MockPrepareRendererResource : public IPrepareRendererResources {
public:
	struct LoadThreadResult {
		const CesiumGltf::Model& model;
        const glm::dmat4& transform;
	};

	struct MainThreadResult {
        const Tile& tile;
        void* pLoadThreadResult;
	};

	struct LoadThreadRasterResult {
		const CesiumGltf::ImageCesium& image;
	};

	struct MainThreadRasterResult {
		const RasterOverlayTile& rasterTile;
		void* pLoadThreadResult;
	};

	virtual void* prepareInLoadThread(
		const CesiumGltf::Model& model,
		const glm::dmat4& transform) override 
	{
		return new LoadThreadResult{model, transform};
	}

	virtual void* prepareInMainThread(Tile& tile, void* pLoadThreadResult) override 
	{
		return new MainThreadResult{tile, pLoadThreadResult};
    }

	virtual void free(
		Tile& tile,
		void* pLoadThreadResult,
		void* pMainThreadResult) noexcept override
	{
		if (pMainThreadResult) {
            MainThreadResult* mainThreadResult = reinterpret_cast<MainThreadResult*>(pMainThreadResult); 
			delete mainThreadResult;
		}

		if (pLoadThreadResult) {
            LoadThreadResult* loadThreadResult = reinterpret_cast<LoadThreadResult*>(pLoadThreadResult); 
			delete loadThreadResult;
		}
	}

	virtual void* prepareRasterInLoadThread(const CesiumGltf::ImageCesium& image) override 
	{
		return new LoadThreadRasterResult{image};
	}

	virtual void* prepareRasterInMainThread(
            const RasterOverlayTile& rasterTile,
            void* pLoadThreadResult) override 
	{
		return new MainThreadRasterResult{rasterTile, pLoadThreadResult};
	}

	virtual void freeRaster(
            const RasterOverlayTile& rasterTile,
            void* pLoadThreadResult,
            void* pMainThreadResult) noexcept override 
	{
		if (pMainThreadResult) {
            MainThreadRasterResult* mainThreadResult = reinterpret_cast<MainThreadRasterResult*>(pMainThreadResult); 
			delete mainThreadResult;
		}

		if (pLoadThreadResult) {
            LoadThreadRasterResult* loadThreadResult = reinterpret_cast<LoadThreadRasterResult*>(pLoadThreadResult); 
			delete loadThreadResult;
		}
	}

	virtual void attachRasterInMainThread(
		const Tile& /*tile*/,
		uint32_t /*overlayTextureCoordinateID*/,
		const RasterOverlayTile& /*rasterTile*/,
		void* /*pMainThreadRendererResources*/,
		const CesiumGeometry::Rectangle& /*textureCoordinateRectangle*/,
		const glm::dvec2& /*translation*/,
		const glm::dvec2& /*scale*/) override 
	{
	}

	virtual void detachRasterInMainThread(
		const Tile& /*tile*/,
		uint32_t /*overlayTextureCoordinateID*/,
		const RasterOverlayTile& /*rasterTile*/,
		void* /*pMainThreadRendererResources*/,
		const CesiumGeometry::Rectangle& /*textureCoordinateRectangle*/) noexcept override 
	{
	}
};


TEST_CASE("Test tileset selection for render") {
  const Ellipsoid& ellipsoid = Ellipsoid::WGS84; 
	Cartographic viewPositionCartographic{
		Math::degreesToRadians(118.0), 
		Math::degreesToRadians(32.0), 
		200.0};
	Cartographic viewFocusCartographic{
		viewPositionCartographic.longitude + Math::degreesToRadians(0.5), 
		viewPositionCartographic.latitude + Math::degreesToRadians(0.5), 
		0.0};
	glm::dvec3 viewPosition = ellipsoid.cartographicToCartesian(viewFocusCartographic);
    glm::dvec3 viewFocus = ellipsoid.cartographicToCartesian(viewFocusCartographic);
	glm::dvec3 viewUp{0.0, 0.0, 1.0};
    glm::dvec2 viewPortSize{500.0, 500.0};

	double aspectRatio = viewPortSize.x / viewPortSize.y;
    double horizontalFieldOfView = Math::degreesToRadians(60.0);
    double verticalFieldOfView = std::atan(std::tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;
	ViewState viewState = ViewState::create(
		viewPosition,
		glm::normalize(viewFocus - viewPosition),
		viewUp,
		viewPortSize,
		horizontalFieldOfView,
		verticalFieldOfView);

	TilesetExternals tilesetExterals {

	};

	Tileset tileset();
}