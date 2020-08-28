#pragma once

#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Gltf.h"
#include <memory>

namespace Cesium3DTiles {
    
    class RasterOverlayTileRequest {
    public:
        RasterOverlayTileRequest(std::unique_ptr<IAssetRequest>&& pAssetRequest);

        typedef void ImageReadyCallback(tinygltf::Image&& image);
        void bind(std::function<ImageReadyCallback> callback);

    private:
        void imageReceived(IAssetRequest* pRequest);

        std::unique_ptr<IAssetRequest> _pAssetRequest;
        std::function<ImageReadyCallback> _callback;
    };

}
