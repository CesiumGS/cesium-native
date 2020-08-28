#include "Cesium3DTiles/RasterOverlayTileRequest.h"
#include "Cesium3DTiles/IAssetResponse.h"

namespace Cesium3DTiles {

    RasterOverlayTileRequest::RasterOverlayTileRequest(std::unique_ptr<IAssetRequest>&& pAssetRequest) :
        _pAssetRequest(std::move(pAssetRequest))
    {
        this->_pAssetRequest->bind(std::bind(&RasterOverlayTileRequest::imageReceived, this, std::placeholders::_1));
    }

    void RasterOverlayTileRequest::bind(std::function<ImageReadyCallback> callback) {
        this->_callback = callback;
    }

    void RasterOverlayTileRequest::imageReceived(IAssetRequest* pRequest) {
        // TODO: dispatch image decoding to a separate thread.
        IAssetResponse* pResponse = pRequest->response();
        
        tinygltf::Image image;
        std::string errors;
        std::string warnings;

        gsl::span<const uint8_t> data = pResponse->data();
        tinygltf::LoadImageData(&image, 0, &errors, &warnings, 0, 0, data.data(), static_cast<int>(data.size()), nullptr);

        // TODO: load failure?

        this->_callback(std::move(image));
    }

}
