#pragma once

#include "CesiumGeospatial/WebMercatorProjection.h"
#include <variant>

namespace CesiumGeospatial {

    typedef std::variant<WebMercatorProjection> Projection;

}
