#pragma once

#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include <functional>

namespace Cesium3DTiles {

    /**
     * @brief Styles of Bing maps overlays
     *
     * Constants that can be passed to a {@link BingMapsRasterOverlay} to
     * indicate the overlays that should be painted.
     */
    struct BingMapsStyle {
        /**
         * @brief Aerial imagery.
         */
        static const std::string AERIAL;

        /**
         * @brief Aerial imagery with a road overlay.
         * @deprecated See https://github.com/CesiumGS/cesium/issues/7128.
         * Use `BingMapsStyle.AERIAL_WITH_LABELS_ON_DEMAND` instead
         */
        static const std::string AERIAL_WITH_LABELS;

        /**
         * @brief Aerial imagery with a road overlay.
         */
        static const std::string AERIAL_WITH_LABELS_ON_DEMAND;

        /**
         * @brief Roads without additional imagery.
         * @deprecated See https://github.com/CesiumGS/cesium/issues/7128.
         * Use `BingMapsStyle.ROAD_ON_DEMAND` instead
         */
        static const std::string ROAD;

        /**
         * @brief Roads without additional imagery.
         */
        static const std::string ROAD_ON_DEMAND;

        /**
         * @brief A dark version of the road maps.
         */
        static const std::string CANVAS_DARK;

        /**
         * @brief A lighter version of the road maps.
         */
        static const std::string CANVAS_LIGHT;

        /**
         * @brief A grayscale version of the road maps.
         */
        static const std::string CANVAS_GRAY;

        /**
         * @brief Ordnance Survey imagery. 
         * 
         * This imagery is visible only for the London, UK area.
         */
        static const std::string ORDNANCE_SURVEY;

        /**
         * @brief Collins Bart imagery.
         */
        static const std::string COLLINS_BART;
    };

    /**
     * @brief A {@link RasterOverlay} that uses Bing maps as the source for the imagery data.
     */
    class CESIUM3DTILES_API BingMapsRasterOverlay : public RasterOverlay {
    public:
        BingMapsRasterOverlay(
            const std::string& url,
            const std::string& key,
            const std::string& mapStyle = BingMapsStyle::AERIAL,
            const std::string& culture = "",
            const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84
        );
        virtual ~BingMapsRasterOverlay() override;

        virtual void createTileProvider(TilesetExternals& tilesetExternals, std::function<CreateTileProviderCallback>&& callback) override;

    private:
        std::string _url;
        std::string _key;
        std::string _mapStyle;
        std::string _culture;
        CesiumGeospatial::Ellipsoid _ellipsoid;

        std::unique_ptr<IAssetRequest> _pMetadataRequest;
        std::function<CreateTileProviderCallback> _callback;
    };

}
