#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumGeospatial/Ellipsoid.h"

namespace Cesium3DTiles {

    struct BingMapsStyle {
        /**
         * Aerial imagery.
         */
        static const std::string AERIAL;

        /**
         * Aerial imagery with a road overlay.
         * @deprecated See https://github.com/CesiumGS/cesium/issues/7128.
         * Use `BingMapsStyle.AERIAL_WITH_LABELS_ON_DEMAND` instead
         */
        static const std::string AERIAL_WITH_LABELS;

        /**
         * Aerial imagery with a road overlay.
         */
        static const std::string AERIAL_WITH_LABELS_ON_DEMAND;

        /**
         * Roads without additional imagery.         *
         * @deprecated See https://github.com/CesiumGS/cesium/issues/7128.
         * Use `BingMapsStyle.ROAD_ON_DEMAND` instead
         */
        static const std::string ROAD;

        /**
         * Roads without additional imagery.
         */
        static const std::string ROAD_ON_DEMAND;

        /**
         * A dark version of the road maps.
         */
        static const std::string CANVAS_DARK;

        /**
         * A lighter version of the road maps.
         */
        static const std::string CANVAS_LIGHT;

        /**
         * A grayscale version of the road maps.
         */
        static const std::string CANVAS_GRAY;

        /**
         * Ordnance Survey imagery. This imagery is visible only for the London, UK area.
         */
        static const std::string ORDNANCE_SURVEY;

        /**
         * Collins Bart imagery.
         */
        static const std::string COLLINS_BART;
    };

    class CESIUM3DTILES_API BingMapsRasterOverlay : public RasterOverlay {
    public:
        BingMapsRasterOverlay(
            const std::string& url,
            const std::string& key,
            const std::string& mapStyle = BingMapsStyle::AERIAL,
            const std::string& culture = "",
            const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84);
        virtual ~BingMapsRasterOverlay() override;

        virtual void createTileProvider(IAssetAccessor& assetAccessor, std::function<CreateTileProviderCallback> callback) override;

    private:
        std::string _url;
        std::string _key;
        std::string _mapStyle;
        std::string _culture;
        CesiumGeospatial::Ellipsoid _ellipsoid;
    };

}
