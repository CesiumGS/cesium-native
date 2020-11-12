#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include <memory>
#include <atomic>

namespace Cesium3DTiles {

    class RasterOverlay;

    class RasterOverlayTile {
    public:
        enum class LoadState {
            Placeholder = -2,

            Failed = -1,

            Unloaded = 0,

            Loading = 1,

            Loaded = 2,

            Done = 3
        };

        /**
         * Constructs a placeholder tile for the tile provider.
         * 
         * @param tileProvider The tile provider.
         */
        RasterOverlayTile(
            RasterOverlay& overlay
        );

        RasterOverlayTile(
            RasterOverlay& overlay,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        );

        ~RasterOverlayTile();

        void load(std::shared_ptr<RasterOverlayTile>& pThis);

        RasterOverlay& getOverlay() { return *this->_pOverlay; }
        const CesiumGeometry::QuadtreeTileID& getID() { return this->_tileID; }
        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }
        const tinygltf::Image& getImage() const { return this->_image; }
        void loadInMainThread();

        void* getRendererResources() const { return this->_pRendererResources; }
        void setRendererResources(void* pValue) { this->_pRendererResources = pValue; }

    private:
        void requestComplete(IAssetRequest* pRequest);
        void setState(LoadState newState);

        RasterOverlay* _pOverlay;
        CesiumGeometry::QuadtreeTileID _tileID;
        std::atomic<LoadState> _state;
        std::unique_ptr<IAssetRequest> _pImageRequest;
        tinygltf::Image _image;
        void* _pRendererResources;
        std::shared_ptr<RasterOverlayTile> _pSelf;
    };
}
