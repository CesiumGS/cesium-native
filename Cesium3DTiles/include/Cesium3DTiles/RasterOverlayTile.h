#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include <memory>
#include <atomic>

namespace Cesium3DTiles {

    class RasterOverlayTileProvider;

    class RasterOverlayTile {
    public:
        enum class LoadState {
            Destroying = -2,

            Failed = -1,

            Unloaded = 0,

            Loading = 1,

            Loaded = 2,

            Done = 3
        };

        RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        );

        RasterOverlayTileProvider& getTileProvider() { return *this->_pTileProvider; }
        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }
        const tinygltf::Image& getImage() const { return this->_image; }
        void loadInMainThread();

        void* getRendererResources() { return this->_pRendererResources; }
        void setRendererResources(void* pValue) { this->_pRendererResources = pValue; }

    private:
        void requestComplete(IAssetRequest* pRequest);
        void setState(LoadState newState);

        RasterOverlayTileProvider* _pTileProvider;
        CesiumGeometry::QuadtreeTileID _tileID;
        std::atomic<LoadState> _state;
        std::unique_ptr<IAssetRequest> _pImageRequest;
        tinygltf::Image _image;
        void* _pRendererResources;
    };
}
