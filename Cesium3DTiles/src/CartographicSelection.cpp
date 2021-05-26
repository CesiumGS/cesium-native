
#include "Cesium3DTiles/CartographicSelection.h"

using namespace CesiumGeospatial;

namespace Cesium3DTiles {

static std::vector<uint32_t> triangulatePolygon(const std::vector<glm::dvec2>& /*polygon*/) {
    std::vector<uint32_t> indices;
    // do stuff
    return indices;
}

static std::optional<GlobeRectangle> computeBoundingRectangle(const std::vector<glm::dvec2>& /*polygon*/) {
    return std::nullopt;
}

CartographicSelection::CartographicSelection(const std::vector<glm::dvec2>& polygon) :
    _vertices(polygon),
    _indices(triangulatePolygon(polygon)),
    _boundingRectangle(computeBoundingRectangle(polygon))
{}

} // namespace Cesium3DTiles