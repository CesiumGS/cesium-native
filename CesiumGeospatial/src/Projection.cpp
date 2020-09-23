#include "CesiumGeospatial/Projection.h"
#include <cmath>

namespace CesiumGeospatial {

    glm::dvec3 projectPosition(const Projection& projection, const Cartographic& position) {
        struct Operation {
            const Cartographic& position;

            glm::dvec3 operator()(const GeographicProjection& geographic) {
                return geographic.project(position);
            }

            glm::dvec3 operator()(const WebMercatorProjection& webMercator) {
                return webMercator.project(position);
            }
        };

        return std::visit(Operation { position }, projection);
    }

    Cartographic unprojectPosition(const Projection& projection, const glm::dvec3& position) {
        struct Operation {
            const glm::dvec3& position;

            Cartographic operator()(const GeographicProjection& geographic) {
                return geographic.unproject(position);
            }

            Cartographic operator()(const WebMercatorProjection& webMercator) {
                return webMercator.unproject(position);
            }
        };

        return std::visit(Operation { position }, projection);
    }

    CesiumGeometry::Rectangle projectRectangleSimple(const Projection& projection, const GlobeRectangle& rectangle) {
        struct Operation {
            const GlobeRectangle& rectangle;

            CesiumGeometry::Rectangle operator()(const GeographicProjection& geographic) {
                return geographic.project(rectangle);
            }

            CesiumGeometry::Rectangle operator()(const WebMercatorProjection& webMercator) {
                return webMercator.project(rectangle);
            }
        };

        return std::visit(Operation { rectangle }, projection);
    }

    GlobeRectangle unprojectRectangleSimple(const Projection& projection, const CesiumGeometry::Rectangle& rectangle) {
        struct Operation {
            const CesiumGeometry::Rectangle& rectangle;

            GlobeRectangle operator()(const GeographicProjection& geographic) {
                return geographic.unproject(rectangle);
            }

            GlobeRectangle operator()(const WebMercatorProjection& webMercator) {
                return webMercator.unproject(rectangle);
            }
        };

        return std::visit(Operation { rectangle }, projection);
    }

    double computeApproximateConversionFactorToMetersNearPosition(const Projection& projection, const glm::dvec2& position) {
        struct Operation {
            const glm::dvec2& position;

            double operator()(const GeographicProjection& /*geographic*/) {
                return 1.0;
            }

            double operator()(const WebMercatorProjection& webMercator) {
                // TODO: is there a better estimate?
                return std::cos(webMercator.unproject(position).latitude);
            }
        };

        return std::visit(Operation { position }, projection);
    }
}
