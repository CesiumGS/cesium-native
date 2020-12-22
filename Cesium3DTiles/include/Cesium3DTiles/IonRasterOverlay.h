#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include <functional>

namespace Cesium3DTiles {

    /**
     * @brief A {@link RasterOverlay} that obtains imagery data from Cesium ion.
     */
    class CESIUM3DTILES_API IonRasterOverlay final : public RasterOverlay {
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

        virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>> createTileProvider(
            const CesiumAsync::AsyncSystem& asyncSystem,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            std::shared_ptr<spdlog::logger> pLogger,
            RasterOverlay* pOwner
        ) override;

    private:
        uint32_t _ionAssetID;
        std::string _ionAccessToken;
    };

}
