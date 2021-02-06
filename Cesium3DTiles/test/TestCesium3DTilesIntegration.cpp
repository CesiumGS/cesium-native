#include "catch2/catch.hpp"

#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetExternals.h"

#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/spdlog-cesium.h"

#include "FileAssetAccessor.h"
#include "NullResourcePreparer.h"
#include "SimpleTaskProcessor.h"
#include "Cesium3DTilesTestUtils.h"

#include <glm/gtx/string_cast.hpp>

using namespace Cesium3DTiles;

TEST_CASE("Some sort of integration test") {

	std::string tilesetUrl = "../Cesium3DTiles/test/Data/Icopsheres/tileset.json";

	// XXX This depends on where all this is built. Have to figure out
	// how to solve this generically. FWIW, my directory name is...
	tilesetUrl = "C:/Develop/CesiumUnreal/Code/cesium-cpp/Data/Icospheres/tileset.json";

	try
	{
		Cesium3DTiles::registerAllTileContentTypes();
		Cesium3DTiles::TilesetExternals externals{
			std::make_shared<Cesium3DTilesTests::FileAssetAccessor>(),
			std::make_shared<Cesium3DTilesTests::NullResourcePreparer>(),
			std::make_shared<Cesium3DTilesTests::SimpleTaskProcessor>()
		};
		Cesium3DTiles::TilesetOptions options;
		options.maximumScreenSpaceError = 100.0;
		Cesium3DTiles::Tileset tileset(externals, tilesetUrl, options);

		// XXX Some hard-wired values at which (for a certain screen
		// size...) the different LOD levels should appear...
		const glm::dvec3 direction{ 0.0, 0.0, -1.0 };
		const glm::dvec3 position0{ 0.0, 0.0, 100.0 };
		const glm::dvec3 position1{ 0.0, 0.0, 20.0 };
		const glm::dvec3 position2{ 0.0, 0.0, 10.0 };
		const glm::dvec3 position3{ 0.0, 0.0, 3.0 };
		std::vector<glm::dvec3> positions{ position0, position1, position2, position3 };

		for (const auto& position : positions) {
			SPDLOG_INFO("Camera at {}", glm::to_string(position));

			const int32_t loadDurationMs = 2500;
			Cesium3DTilesTests::sleepMsLogged(loadDurationMs);

			Cesium3DTiles::ViewState viewState = Cesium3DTilesTests::createViewState(position, direction);
			Cesium3DTiles::ViewUpdateResult r = tileset.updateView(viewState);
			Cesium3DTilesTests::printViewUpdateResult(r);
		}
	}
	catch (const std::exception& e) {
		SPDLOG_CRITICAL("Unhandled error: {0}", e.what());
	}
}
