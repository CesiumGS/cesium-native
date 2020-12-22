#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/RasterOverlayCutoutCollection.h"
#include "CesiumAsync/AsyncSystem.h"
#include <spdlog/fwd.h>
#include <functional>
#include <memory>
#include <optional>

namespace Cesium3DTiles {

    class IPrepareRendererResources;
    class RasterOverlayTileProvider;
    class RasterOverlayCollection;

    /**
     * @brief The base class for a quadtree-tiled raster image that can be draped over a {@link Tileset}.
     * 
     * Instances of this class can be added to the {@link RasterOverlayCollection}
     * that is returned by {@link Tileset::getOverlays}.
     *
     * @see BingMapsRasterOverlay
     * @see IonRasterOverlay
     * @see TileMapServiceRasterOverlay     
     */
    class RasterOverlay {
    public:
        RasterOverlay();
        virtual ~RasterOverlay();

        /**
         * @brief Gets the tile provider for this overlay.
         * 
         * @return `nullptr` if {@link createTileProvider} has not yet been called.
         * If {@link createTileProvider} has been called but the overlay is not yet ready to
         * provide tiles, a placeholder tile provider will be returned.
         */
        RasterOverlayTileProvider* getTileProvider() noexcept;

        /** @copydoc getTileProvider */
        const RasterOverlayTileProvider* getTileProvider() const noexcept;

        /**
         * @brief Gets the placeholder tile provider for this overlay.
         * 
         * @return `nullptr` if {@link createTileProvider} has not yet been called.
         */
        RasterOverlayTileProvider* getPlaceholder() noexcept { return this->_pPlaceholder.get(); }

        /** @copydoc getPlaceholder */
        const RasterOverlayTileProvider* getPlaceholder() const noexcept { return this->_pPlaceholder.get(); }

        /**
         * @brief Get a collection containing the sections of this overlay and its associated tileset that are not rendered.
         */
        RasterOverlayCutoutCollection& getCutouts() noexcept { return this->_cutouts; }

        /** @copydoc getCutouts */
        const RasterOverlayCutoutCollection& getCutouts() const noexcept { return this->_cutouts; }

        /**
         * @brief Returns whether this overlay is in the process of being destroyed.
         */
        bool isBeingDestroyed() const noexcept { return this->_pSelf != nullptr; }

        /**
         * @brief A callback that receives the tile provider when it asynchronously becomes ready.
         * 
         * @param pTileProvider The newly-created tile provider.
         */
        typedef void CreateTileProviderCallback(std::unique_ptr<RasterOverlayTileProvider>&& pTileProvider);

        /**
         * @brief Begins asynchronous creation of the tile provider for this overlay and eventually makes it available directly from this instance.
         * 
         * When the tile provider is ready, it will be returned by {@link getTileProvider}.
         * 
         * This method does nothing if the tile provider has already been created or
         * is already in the process of being created.
         * 
         * @param asyncSystem The async system used to request assets and do work in threads.
         * @param pCreditSystem The {@link CreditSystem} to use when creating a per-TileProvider {@link Credit}.
         * @param pPrepareRendererResources The interface used to prepare raster images for rendering.
         */
        void createTileProvider(
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::shared_ptr<CreditSystem>& pCreditSystem,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            std::shared_ptr<spdlog::logger> pLogger
        );

        /**
         * @brief Begins asynchronous creation of the tile provider for this overlay and eventually returns it via a Future.
         * 
         * The created tile provider will not be returned via {@link getTileProvider}. This method is primarily useful for
         * overlays that aggregate other overlays.
         * 
         * @param asyncSystem The async system used to request assets and do work in threads.
         * @param pCreditSystem The {@link CreditSystem} to use when creating a per-TileProvider {@link Credit}.
         * @param pPrepareRendererResources The interface used to prepare raster images for rendering.
         * @param pOwner The overlay that owns this overlay, or nullptr if this overlay is not aggregated.
         * @param callback The callback that receives the new tile provider when it is ready.
         */
        virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>> createTileProvider(
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::shared_ptr<CreditSystem>& pCreditSystem,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            std::shared_ptr<spdlog::logger> pLogger,
            RasterOverlay* pOwner
        ) = 0;

        /**
         * @brief Safely destroys this overlay.
         * 
         * This method is not supposed to be called by clients.
         * The overlay will not be truly destroyed until all in-progress tile loads complete. This may happen
         * before this function returns if no loads are in progress.
         * 
         * @param pOverlay A unique pointer to this instance, allowing transfer of ownership.
         */
        void destroySafely(std::unique_ptr<RasterOverlay>&& pOverlay) noexcept;
    
    private:
        std::unique_ptr<RasterOverlayTileProvider> _pPlaceholder;
        std::unique_ptr<RasterOverlayTileProvider> _pTileProvider;
        RasterOverlayCutoutCollection _cutouts;
        std::unique_ptr<RasterOverlay> _pSelf;
        bool _isLoadingTileProvider;
    };

}
