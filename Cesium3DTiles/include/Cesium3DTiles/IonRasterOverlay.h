#pragma once

#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include <functional>

namespace Cesium3DTiles {

    /**
     * @brief A {@link RasterOverlay} that obtains imagery data from Cesium ion.
     */
    class CESIUM3DTILES_API IonRasterOverlay : public RasterOverlay {
    public:

        /**
         * @brief Creates a new instance.
         * 
         * The tiles that are provided by this instance will contain
         * imagery data that was obtained from the Cesium ion asset
         * with the given ID, accessed with the given access token.
         * 
         * @param ionAssetID The asset ID.
         * @param ionAccessToken The access token.
         */
        IonRasterOverlay(
            uint32_t ionAssetID,
            const std::string& ionAccessToken
        );
        virtual ~IonRasterOverlay() override;

        virtual void createTileProvider(TilesetExternals& tilesetExternals, std::function<CreateTileProviderCallback>&& callback) override;

    private:
        uint32_t _ionAssetID;
        std::string _ionAccessToken;

        std::unique_ptr<IAssetRequest> _pMetadataRequest;
        std::function<CreateTileProviderCallback> _callback;
        std::unique_ptr<RasterOverlay> _aggregatedOverlay;
    };

}
