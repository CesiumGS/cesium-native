#include "catch2/catch.hpp"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "glm/mat4x4.hpp"
#include <fstream>
#include <filesystem>

using namespace CesiumAsync;
using namespace Cesium3DTiles;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

class MockAssetResponse : public IAssetResponse {
public:
	MockAssetResponse(uint16_t statusCode,
		const std::string& contentType,
		const std::vector<uint8_t>& data)
		: mockStatusCode{ statusCode }
		, mockContentType{ contentType }
		, mockData{ data }
	{}

	virtual uint16_t statusCode() const override { return this->mockStatusCode; }

	virtual std::string contentType() const override { return this->mockContentType; }

	virtual gsl::span<const uint8_t> data() const override {
		return gsl::span<const uint8_t>(mockData.data(), mockData.size());
	}

	uint16_t mockStatusCode;
	std::string mockContentType;
	std::vector<uint8_t> mockData;
};

class MockAssetRequest : public IAssetRequest {
public:
	MockAssetRequest(const std::string& url,
		std::unique_ptr<MockAssetResponse> response)
		: requestUrl{ url }
		, pResponse{ std::move(response) }
	{}

	virtual std::string url() const override {
		return this->requestUrl;
	}

	virtual IAssetResponse* response() override {
		return this->pResponse.get();
	}

	virtual void bind(std::function<void(IAssetRequest*)> callback) override {
		callback(this);
	}

	virtual void cancel() noexcept override {}

	std::string requestUrl;
	std::unique_ptr<MockAssetResponse> pResponse;
};

class MockAssetAccessor : public IAssetAccessor {
public:
	MockAssetAccessor(std::map<std::string, std::unique_ptr<MockAssetRequest>> mockCompletedRequests)
		: mockCompletedRequests{std::move(mockCompletedRequests)}
	{}

	virtual std::unique_ptr<IAssetRequest> requestAsset(
		const std::string& url,
		const std::vector<THeader>&) override
	{
		auto mockRequestIt = mockCompletedRequests.find(url);
		if (mockRequestIt != mockCompletedRequests.end()) {
			return std::move(mockRequestIt->second);
		}

		return nullptr;
	}

	virtual void tick() noexcept override {}

	std::map<std::string, std::unique_ptr<MockAssetRequest>> mockCompletedRequests;
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
		Tile& /*tile*/,
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
            const RasterOverlayTile& /*rasterTile*/,
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

static std::vector<uint8_t> readFile(const std::filesystem::path& fileName) {
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	REQUIRE(file);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	file.read(reinterpret_cast<char*>(buffer.data()), size);

	return buffer;
}

static bool isTileMeetSSE(const ViewState& viewState, const Tile& tile, const Tileset& tileset) {
	double distance = glm::sqrt(viewState.computeDistanceSquaredToBoundingVolume(tile.getBoundingVolume()));
	double sse = viewState.computeScreenSpaceError(tile.getGeometricError(), distance);
	return sse < tileset.getOptions().maximumScreenSpaceError;
}

static void initializeTileset(Tileset& tileset) {
	// create a random view state so that we can able to load the tileset first
	const Ellipsoid& ellipsoid = Ellipsoid::WGS84; 
	Cartographic viewPositionCartographic{
		Math::degreesToRadians(118.0), 
		Math::degreesToRadians(32.0), 
		200.0};
	Cartographic viewFocusCartographic{
		viewPositionCartographic.longitude + Math::degreesToRadians(0.5), 
		viewPositionCartographic.latitude + Math::degreesToRadians(0.5), 
		0.0};
	glm::dvec3 viewPosition = ellipsoid.cartographicToCartesian(viewPositionCartographic);
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

	tileset.updateView(viewState);
}

static ViewState zoomToTileset(const Tileset& tileset) {
	const Tile* root = tileset.getRootTile();
	REQUIRE(root != nullptr);

	const BoundingRegion* region = std::get_if<BoundingRegion>(&root->getBoundingVolume());
	REQUIRE(region != nullptr);

	const GlobeRectangle& rectangle = region->getRectangle();
	double maxHeight = region->getMaximumHeight();
	Cartographic center = rectangle.computeCenter();
	Cartographic corner = rectangle.getNorthwest();
	corner.height = maxHeight;

	const Ellipsoid& ellipsoid = Ellipsoid::WGS84; 
	glm::dvec3 viewPosition = ellipsoid.cartographicToCartesian(corner);
    glm::dvec3 viewFocus = ellipsoid.cartographicToCartesian(center);
	glm::dvec3 viewUp{0.0, 0.0, 1.0};
    glm::dvec2 viewPortSize{500.0, 500.0};
	double aspectRatio = viewPortSize.x / viewPortSize.y;
    double horizontalFieldOfView = Math::degreesToRadians(60.0);
    double verticalFieldOfView = std::atan(std::tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;
	return ViewState::create(
		viewPosition,
		glm::normalize(viewFocus - viewPosition),
		viewUp,
		viewPortSize,
		horizontalFieldOfView,
		verticalFieldOfView);
}

TEST_CASE("Test replace refinement for render") {
	// initialize tileset
	std::filesystem::path testDataPath = "C:\\Users\\bao\\Documents\\Projects\\cesium-unreal-demo\\Plugins\\cesium-unreal\\extern\\cesium-native\\Cesium3DTiles\\test\\data";
	testDataPath = testDataPath / "Tileset";
	std::vector<std::string> files{
		"tileset.json",
		"parent.b3dm",
		"ll.b3dm",
		"lr.b3dm",
		"ul.b3dm",
		"ur.b3dm",
	};

	std::map<std::string, std::unique_ptr<MockAssetRequest>> mockCompletedRequests;
	for (const auto& file : files) {
		mockCompletedRequests.insert({ 
			file, 
			std::make_unique<MockAssetRequest>(file, std::make_unique<MockAssetResponse>(static_cast<uint16_t>(200), "app/json", readFile(testDataPath / file))) });
	}

	std::shared_ptr<MockAssetAccessor> mockAssetAccessor = std::make_shared<MockAssetAccessor>(std::move(mockCompletedRequests));
	TilesetExternals tilesetExternals {
		mockAssetAccessor,
		std::make_shared<MockPrepareRendererResource>(),
        std::make_shared<MockTaskProcessor>()
	};

	// create tileset and call updateView() to give it a chance to load
	std::unique_ptr<Tileset> tileset = std::make_unique<Tileset>(tilesetExternals, "tileset.json");
	initializeTileset(*tileset);

	// check the tiles status
	const Tile* root = tileset->getRootTile();
	REQUIRE(root->getState() == Tile::LoadState::ContentLoading);
	for (const auto& child : root->getChildren()) {
		REQUIRE(child.getState() == Tile::LoadState::Unloaded);
	}

	SECTION("Test no refinement happen when tile meet SSE") {
		// Zoom to tileset. Expect the root will not meet sse in this configure
		ViewState viewState = zoomToTileset(*tileset);

		// Zoom out from the tileset a little bit to make sure the root meet sse
		glm::dvec3 zoomOutPosition = viewState.getPosition() - viewState.getDirection() * 2500.0;
		ViewState zoomOutViewState = ViewState::create(zoomOutPosition,
			viewState.getDirection(),
			viewState.getUp(),
			viewState.getViewportSize(),
			viewState.getHorizontalFieldOfView(),
			viewState.getVerticalFieldOfView());

		// Check 1st and 2nd frame. Root should meet sse and render. No transitions are
		// expected here
		for (int frame = 0; frame < 2; ++frame) {
			ViewUpdateResult result = tileset->updateView(zoomOutViewState);

			// Check tile state. Ensure root meet sse
			REQUIRE(root->getState() == Tile::LoadState::Done);
			REQUIRE(isTileMeetSSE(zoomOutViewState, *root, *tileset));
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::Unloaded);
			}

			// check result
			REQUIRE(result.tilesToRenderThisFrame.size() == 1);
			REQUIRE(result.tilesToRenderThisFrame.front() == root);

			REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

			REQUIRE(result.tilesVisited == 1);
			REQUIRE(result.tilesLoadingMediumPriority == 0);
			REQUIRE(result.tilesCulled == 0);
			REQUIRE(result.culledTilesVisited == 0);

			// check children state are still unloaded
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::Unloaded);
			}
		}
	}

	SECTION("Test root doesn't meet sse but has to be rendered because children cannot be rendered") {
		// we should forbit hole first to let the checks below happen
		tileset->getOptions().forbidHoles = true;

		SECTION("Children cannot be rendered because of no response") {
			// remove one of children completed response to mock network error
			mockAssetAccessor->mockCompletedRequests["ll.b3dm"]->pResponse = nullptr;
			mockAssetAccessor->mockCompletedRequests["lr.b3dm"]->pResponse = nullptr;
			mockAssetAccessor->mockCompletedRequests["ul.b3dm"]->pResponse = nullptr;
			mockAssetAccessor->mockCompletedRequests["ur.b3dm"]->pResponse = nullptr;
		}

		SECTION("Children cannot be rendered because response has an failed status code") {
			// remove one of children completed response to mock network error
			mockAssetAccessor->mockCompletedRequests["ll.b3dm"]->pResponse->mockStatusCode = 404;
			mockAssetAccessor->mockCompletedRequests["lr.b3dm"]->pResponse->mockStatusCode = 404;
			mockAssetAccessor->mockCompletedRequests["ul.b3dm"]->pResponse->mockStatusCode = 404;
			mockAssetAccessor->mockCompletedRequests["ur.b3dm"]->pResponse->mockStatusCode = 404;
		}

		ViewState viewState = zoomToTileset(*tileset);

		// 1st frame. Root doesn't meet sse, so it goes to children. But because children
		// haven't started loading, root should be rendered. 
		{
			ViewUpdateResult result = tileset->updateView(viewState);

			// Check tile state. Ensure root doesn't meet sse, but children does. Children begin
			// loading as well
			REQUIRE(root->getState() == Tile::LoadState::Done);
			REQUIRE(!isTileMeetSSE(viewState, *root, *tileset));
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
				REQUIRE(isTileMeetSSE(viewState, child, *tileset));
			}

			// check result
			REQUIRE(result.tilesToRenderThisFrame.size() == 1);
			REQUIRE(result.tilesToRenderThisFrame.front() == root);

			REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

			REQUIRE(result.tilesVisited == 1);
			REQUIRE(result.tilesLoadingMediumPriority == 4);
			REQUIRE(result.tilesCulled == 0);
			REQUIRE(result.culledTilesVisited == 0);
		}

		// 2nd frame. Because children receive failed response, so they can't be rendered. Root should
		// be rendered instead. Children should have failed load states
		{
			ViewUpdateResult result = tileset->updateView(viewState);

			// Check tile state. Ensure root doesn't meet sse, but children does
			REQUIRE(root->getState() == Tile::LoadState::Done);
			REQUIRE(!isTileMeetSSE(viewState, *root, *tileset));
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::FailedTemporarily);
				REQUIRE(isTileMeetSSE(viewState, child, *tileset));
			}

			// check result
			REQUIRE(result.tilesToRenderThisFrame.size() == 1);
			REQUIRE(result.tilesToRenderThisFrame.front() == root);

			REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

			REQUIRE(result.tilesVisited == 1);
			REQUIRE(result.tilesLoadingMediumPriority == 0);
			REQUIRE(result.tilesCulled == 0);
			REQUIRE(result.culledTilesVisited == 0);
		}
	}

	SECTION("Test parent meets sse but not renderable") {
		// Zoom to tileset. Expect the root will not meet sse in this configure
		ViewState viewState = zoomToTileset(*tileset);

		ViewUpdateResult result = tileset->updateView(viewState);
	}

	SECTION("Test child refinement when parent doesn't meet SSE") {
		ViewState viewState = zoomToTileset(*tileset);

		// 1st frame. Root doesn't meet sse and children does. However, because 
		// none of the children are loaded, root will be rendered instead and children
		// transition from unloaded to loading in the mean time
		{
			ViewUpdateResult result = tileset->updateView(viewState);

			// Check tile state. Ensure root doesn't meet sse, but children does
			REQUIRE(root->getState() == Tile::LoadState::Done);
			REQUIRE(!isTileMeetSSE(viewState, *root, *tileset));
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
				REQUIRE(isTileMeetSSE(viewState, child, *tileset));
			}

			// check result
			REQUIRE(result.tilesToRenderThisFrame.size() == 1);
			REQUIRE(result.tilesToRenderThisFrame.front() == root);

			REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

			REQUIRE(result.tilesVisited == 5);
			REQUIRE(result.tilesLoadingMediumPriority == 4);
			REQUIRE(result.tilesCulled == 0);
			REQUIRE(result.culledTilesVisited == 0);
		}

		// 2nd frame. Children are finished loading and ready to be rendered. Root
		// shouldn't be rendered in this frame
		{
			ViewUpdateResult result = tileset->updateView(viewState);

			// check tile states
			REQUIRE(root->getState() == Tile::LoadState::Done);
			for (const auto& child : root->getChildren()) {
				REQUIRE(child.getState() == Tile::LoadState::Done);
			}

			// check result
			REQUIRE(result.tilesToRenderThisFrame.size() == 4);
			for (const auto& child : root->getChildren()) {
				REQUIRE(std::find(result.tilesToRenderThisFrame.begin(), 
					result.tilesToRenderThisFrame.end(), 
					&child) != result.tilesToRenderThisFrame.end());
			}

			REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 1);
			REQUIRE(result.tilesToNoLongerRenderThisFrame.front() == root);

			REQUIRE(result.tilesVisited == 5);
			REQUIRE(result.tilesLoadingMediumPriority == 0);
			REQUIRE(result.tilesCulled == 0);
			REQUIRE(result.culledTilesVisited == 0);
		}
	}
}