#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include <memory>
#include <atomic>

namespace Cesium3DTiles {

    class RasterOverlayTileProvider;

    /**
     * @brief Raster image data for a tile in a quadtree.
     * 
     * Instances of this clas represent tiles of a quadtree that have
     * an associated image, which us used as an imagery overlay 
     * for tile geometry. The connection between the imagery data
     * and the actual tile geometry is established via the
     * {@link RasterMappedTo3DTile} class, which combines a
     * raster overlay tile with texture coordinates, to map the
     * image on the geometry of a {@link Tile}.
     */
    class RasterOverlayTile {
    public:

        /**
         * @brief Lifecycle states of a raster overlay tile.
         */
        enum class LoadState {

            /**
             * @brief Indicator for a placeholder tile.
             */
            Placeholder = -2,

            /**
             * @brief The image request or image creation failed.
             */
            Failed = -1,

            /**
             * @brief The initial state.
             */
            Unloaded = 0,

            /**
             * @brief The request for loading the image data is still pending.
             */
            Loading = 1,

            /**
             * @brief The image data has been loaded and the image has been created.
             */
            Loaded = 2,

            /**
             * @brief The rendering resources for the image data have been created.
             */
            Done = 3
        };

        /**
         * @brief Constructs a placeholder tile for the tile provider.
         * 
         * The {@link getState} of this instance will always be {@link LoadState::Placeholder}.
         * 
         * @param tileProvider The tile provider.
         */
        RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider
        );

        /**
         * @brief Creates a new instance.
         * 
         * This is called by a {@link RasterOverlayTileProvider} when a new, previously
         * unknown tile is reqested. It receives the request for the image data, and
         * the {@link getState} will initially be {@link LoadState `Loading`}. 
         * The constructor will attach a callback to this request.  When the request 
         * completes successfully and the {@link getImage} data can be created,
         * the state of this instance will change to {@link LoadState `Loaded`}.
         * Otherwise, the state will become {@link LoadState `Failed`}.
         * 
         * @param tileProvider The tile provider (this may be different than the
         * tile provider that calls this constructor).
         * @param tileID The {@link CesiumGeometry::QuadtreeTileID} for this tile.
         * @param pImageRequest The pending request for the image data.
         */
        RasterOverlayTile(
            RasterOverlayTileProvider& tileProvider,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        );

        /** @brief Default destructor. */
        ~RasterOverlayTile();

        /** 
         * @brief Returns the {@link RasterOverlayTileProvider} that was given during construction.
         */
        RasterOverlayTileProvider& getTileProvider() { return *this->_pTileProvider; }
        
        /**
         * @brief Returns the {@link CesiumGeometry::QuadtreeTileID} that was given during construction.
         */
        const CesiumGeometry::QuadtreeTileID& getID() { return this->_tileID; }

        /**
         * @brief Returns the current {@link LoadState}.
         */
        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }

        /**
         * @brief Returns the image data for the tile.
         * 
         * This will only contain valid image data if the {@link getState} of
         * this tile is {@link LoadState `Loaded`} or {@link LoadState `Done`}.
         * 
         * @return The image data.
         */
        const tinygltf::Image& getImage() const { return this->_image; }

        /**
         * @brief Create the renderer resources for the loaded image.
         * 
         * If the {@link getState} of this tile is not {@link LoadState `Loaded`},
         * then nothing will be done. Otherwise, the renderer resources will be
         * prepared, so that they may later be obtained with {@link getRendererResources},
         * and the {@link getState} of this tile will change to {@link LoadState `Done`}.
         */
        void loadInMainThread();

        /**
         * @brief Returns the renderer resources that have been created for this tile.
         */
        void* getRendererResources() const { return this->_pRendererResources; }

        /**
         * @brief Set the renderer resources for this tile.
         * 
         * This function is not supposed to be called by clients.
         */
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
