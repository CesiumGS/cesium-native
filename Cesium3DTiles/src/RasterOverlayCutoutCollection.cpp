// Copyright CesiumGS, Inc. and Contributors

#include "Cesium3DTiles/RasterOverlayCutoutCollection.h"

namespace Cesium3DTiles {

    RasterOverlayCutoutCollection::RasterOverlayCutoutCollection() noexcept :
        _cutouts()
    {
    }

    void RasterOverlayCutoutCollection::push_back(const CesiumGeospatial::GlobeRectangle& cutoutRectangle) {
        this->_cutouts.push_back(cutoutRectangle);
    }

}
