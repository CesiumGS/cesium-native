#include "TileUtilities.h"

#include <variant>

#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h"

namespace Cesium3DTiles {
    namespace Impl {

        const CesiumGeospatial::GlobeRectangle* obtainGlobeRectangle(const Cesium3DTiles::BoundingVolume* boundingVolume) noexcept {
            const CesiumGeospatial::BoundingRegion* pRegion = std::get_if<CesiumGeospatial::BoundingRegion>(boundingVolume);
            if (pRegion) {
                return &pRegion->getRectangle();
            }
            const CesiumGeospatial::BoundingRegionWithLooseFittingHeights* pLooseRegion = std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(boundingVolume);
            if (pLooseRegion) {
                return &pLooseRegion->getBoundingRegion().getRectangle();
            }
            return nullptr;
        }
    }

}
