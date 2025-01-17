#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumIonClient/Geocoder.h>

#include <variant>

namespace CesiumIonClient {

CesiumGeospatial::GlobeRectangle GeocoderFeature::getGlobeRectangle() const {
  struct Operation {
    CesiumGeospatial::GlobeRectangle
    operator()(const CesiumGeospatial::GlobeRectangle& rect) {
      return rect;
    }

    CesiumGeospatial::GlobeRectangle
    operator()(const CesiumGeospatial::Cartographic& cartographic) {
      return CesiumGeospatial::GlobeRectangle(
          cartographic.longitude,
          cartographic.latitude,
          cartographic.longitude,
          cartographic.latitude);
    }
  };

  return std::visit(Operation{}, this->destination);
}

CesiumGeospatial::Cartographic GeocoderFeature::getCartographic() const {
  struct Operation {
    CesiumGeospatial::Cartographic
    operator()(const CesiumGeospatial::GlobeRectangle& rect) {
      return rect.computeCenter();
    }

    CesiumGeospatial::Cartographic
    operator()(const CesiumGeospatial::Cartographic& cartographic) {
      return cartographic;
    }
  };

  return std::visit(Operation{}, this->destination);
}
} // namespace CesiumIonClient