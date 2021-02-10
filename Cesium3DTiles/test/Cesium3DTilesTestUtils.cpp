
#include "Cesium3DTilesTestUtils.h"

#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/ViewUpdateResult.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/Tile.h"

#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/OctreeTileID.h"

#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>


namespace Cesium3DTilesTests 
{
	namespace 
	{
		constexpr double MATH_PI = 3.14159265358979323846;
	}

	Cesium3DTiles::ViewState createViewState(const glm::dvec3& p, const glm::dvec3 d) {
		const glm::dvec3 position{ p };
		const glm::dvec3 direction{ d};
		const glm::dvec3 up{ 0.0, 1.0, 0.0 };
		const glm::dvec2 viewportSize{ 800.0, 600.0 };
		const double horizontalFieldOfView = MATH_PI / 3.0;
		const double verticalFieldOfView = MATH_PI / 3.0;
		Cesium3DTiles::ViewState viewState = Cesium3DTiles::ViewState::create(
			position, direction, up, viewportSize,
			horizontalFieldOfView, verticalFieldOfView);
		return viewState;
	}

	Cesium3DTiles::ViewState createViewState() {
		return createViewState(glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, -1.0 });
	}

	void sleepMsLogged(const int32_t ms) {
	
		if (ms > 0) {
			SPDLOG_TRACE("Sleeping for {0}ms", ms);
			std::chrono::milliseconds duration{ std::chrono::milliseconds(ms) };
			std::this_thread::sleep_for(duration);
			SPDLOG_TRACE("Sleeping for {0}ms DONE", duration.count());
		}

	}

	namespace 
	{
		int32_t computeHeight(Cesium3DTiles::Tile const *tile) {
			int32_t height = 0;
			Cesium3DTiles::Tile const* t = tile;
			while (t != nullptr) {
				height++;
				t = t->getParent();
			}
			return height;
		}

	}

	std::string viewUpdateResultToString(const Cesium3DTiles::ViewUpdateResult& r) {
		int w = 35;
		std::ostringstream s;

		s << std::setw(w) << "tilesToRenderThisFrame : " << r.tilesToRenderThisFrame.size() << std::endl;

		// TODO Here's the option to add more details. Could be a parameter...
		bool details = true;
		if (details) {
			for (auto const& tile : r.tilesToRenderThisFrame) {
				s << std::setw(w) << "" 
					<< "  ID " << createTileIdString(tile->getTileID()) 
					<< " error " << tile->getGeometricError() 
					<< " height " << computeHeight(tile)
					<< std::endl;
			}
		}

		s << std::setw(w) << "tilesToNoLongerRenderThisFrame : " << r.tilesToNoLongerRenderThisFrame.size() << std::endl;
		s << std::setw(w) << "tilesLoadingLowPriority : " << r.tilesLoadingLowPriority << std::endl;
		s << std::setw(w) << "tilesLoadingMediumPriority : " << r.tilesLoadingMediumPriority << std::endl;
		s << std::setw(w) << "tilesLoadingHighPriority : " << r.tilesLoadingHighPriority << std::endl;
		s << std::setw(w) << "tilesVisited : " << r.tilesVisited << std::endl;
		s << std::setw(w) << "tilesCulled : " << r.tilesCulled << std::endl;
		s << std::setw(w) << "maxDepthVisited : " << r.maxDepthVisited << std::endl;
		return s.str();
	}


	void printViewUpdateResult(const Cesium3DTiles::ViewUpdateResult& r) {
		SPDLOG_INFO("ViewUpdateResult:\n" + viewUpdateResultToString(r));
	}

	std::string createTileIdString(const Cesium3DTiles::TileID& tileId) {
		struct Operation {
			std::string operator()(const std::string& id) {
				return id;
			}

			// XXX TODO It's hard to return sensible strings for these...
			std::string operator()(const CesiumGeometry::QuadtreeTileID& /*id*/) {
				return "UNHANDLED IN createTileIdString!";
			}

			std::string operator()(const CesiumGeometry::OctreeTileID& /*id*/) {
				return "UNHANDLED IN createTileIdString!";
			}

			std::string operator()(const CesiumGeometry::UpsampledQuadtreeNode /*id*/) {
				return "UNHANDLED IN createTileIdString!";
			}
		};
		return std::visit(Operation(), tileId);
	}
}
