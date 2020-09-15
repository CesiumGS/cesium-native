#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"

namespace Cesium3DTiles {

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        ) :
            _pTileProvider(&tileProvider),
            _tileID(tileID),
            _state(LoadState::Unloaded),
            _pImageRequest(std::move(pImageRequest)),
            _image(),
            _pRendererResources(nullptr)
        {
            this->_pImageRequest->bind(std::bind(&RasterOverlayTile::requestComplete, this, std::placeholders::_1));
            this->setState(LoadState::Loading);
        }

        void RasterOverlayTile::loadInMainThread() {
            if (this->getState() != RasterOverlayTile::LoadState::Loaded) {
                return;
            }

            // Do the final main thread raster loading
            TilesetExternals& externals = this->_pTileProvider->getExternals();
            this->_pRendererResources = externals.pPrepareRendererResources->prepareRasterInMainThread(*this, this->_pRendererResources);
            this->setState(LoadState::Done);
        }

        void RasterOverlayTile::requestComplete(IAssetRequest* pRequest) {
            // TODO: dispatch image decoding to a separate thread.
            IAssetResponse* pResponse = pRequest->response();
            
            std::string errors;
            std::string warnings;

            gsl::span<const uint8_t> data = pResponse->data();
            tinygltf::LoadImageData(&this->_image, 0, &errors, &warnings, 0, 0, data.data(), static_cast<int>(data.size()), nullptr);

            // TODO: load failure?

            TilesetExternals& externals = this->_pTileProvider->getExternals();
            this->_pRendererResources = externals.pPrepareRendererResources->prepareRasterInLoadThread(*this);

            this->setState(LoadState::Loaded);
            this->_pImageRequest.reset();
        }

        void RasterOverlayTile::setState(LoadState newState) {
            this->_state.store(newState, std::memory_order::memory_order_release);
        }

}
