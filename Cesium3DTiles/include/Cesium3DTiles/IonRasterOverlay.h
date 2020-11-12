#pragma once

#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include <functional>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API IonRasterOverlay : public RasterOverlay {
    public:
        IonRasterOverlay(
            uint32_t ionAssetID,
            const std::string& ionAccessToken
        );
        virtual ~IonRasterOverlay() override;

        virtual void createTileProvider(const TilesetExternals& externals, RasterOverlay* pOwner, std::function<CreateTileProviderCallback>&& callback) override;

    private:
        uint32_t _ionAssetID;
        std::string _ionAccessToken;

        std::unique_ptr<IAssetRequest> _pMetadataRequest;
        std::unique_ptr<RasterOverlay> _aggregatedOverlay;
    };

}
