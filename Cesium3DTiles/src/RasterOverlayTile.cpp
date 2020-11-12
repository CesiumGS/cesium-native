#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/RasterOverlay.h"

namespace Cesium3DTiles {

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlay& overlay
        ) :
            _pOverlay(&overlay),
            _tileID(0, 0, 0),
            _state(LoadState::Placeholder),
            _pImageRequest(),
            _image(),
            _pRendererResources(nullptr),
            _pSelf(nullptr)
        {
        }

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlay& overlay,
            const CesiumGeometry::QuadtreeTileID& tileID,
            std::unique_ptr<IAssetRequest>&& pImageRequest
        ) :
            _pOverlay(&overlay),
            _tileID(tileID),
            _state(LoadState::Unloaded),
            _pImageRequest(std::move(pImageRequest)),
            _image(),
            _pRendererResources(nullptr),
            _pSelf(nullptr)
        {
        }

        RasterOverlayTile::~RasterOverlayTile() {
            RasterOverlayTileProvider* pTileProvider = this->_pOverlay->getTileProvider();
            const TilesetExternals& externals = pTileProvider->getExternals();

            void* pLoadThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? nullptr : this->_pRendererResources;
            void* pMainThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? this->_pRendererResources : nullptr;

            externals.pPrepareRendererResources->freeRaster(*this, pLoadThreadResult, pMainThreadResult);
        }

        void RasterOverlayTile::load(std::shared_ptr<RasterOverlayTile>& pThis) {
            this->_pSelf = pThis;
            this->setState(LoadState::Loading);
            this->_pImageRequest->bind(std::bind(&RasterOverlayTile::requestComplete, this, std::placeholders::_1));
        }

        void RasterOverlayTile::loadInMainThread() {
            if (this->getState() != RasterOverlayTile::LoadState::Loaded) {
                return;
            }

            this->_pImageRequest.reset();

            // Do the final main thread raster loading
            RasterOverlayTileProvider* pTileProvider = this->_pOverlay->getTileProvider();
            const TilesetExternals& externals = pTileProvider->getExternals();
            this->_pRendererResources = externals.pPrepareRendererResources->prepareRasterInMainThread(*this, this->_pRendererResources);
            this->setState(LoadState::Done);
        }

        void RasterOverlayTile::requestComplete(IAssetRequest* pRequest) {
            IAssetResponse* pResponse = pRequest->response();
            if (pResponse == nullptr) {
                this->setState(LoadState::Failed);
                return;
            }

            if (pResponse->data().size() == 0) {
                this->setState(LoadState::Failed);
                return;
            }
            
            RasterOverlayTileProvider* pTileProvider = this->_pOverlay->getTileProvider();
            const TilesetExternals& externals = pTileProvider->getExternals();

            externals.pTaskProcessor->startTask([pResponse, this]() {
                std::string errors;
                std::string warnings;

                gsl::span<const uint8_t> data = pResponse->data();
                bool success = tinygltf::LoadImageData(&this->_image, 0, &errors, &warnings, 0, 0, data.data(), static_cast<int>(data.size()), nullptr);

                const int bytesPerPixel = 4;
                if (success && this->_image.image.size() >= static_cast<size_t>(this->_image.width * this->_image.height * bytesPerPixel)) {
                    RasterOverlay& overlay = this->getOverlay();
                    RasterOverlayTileProvider* pTileProvider = overlay.getTileProvider();

                    CesiumGeometry::Rectangle tileRectangle = pTileProvider->getTilingScheme().tileToRectangle(this->getID());
                    double tileWidth = tileRectangle.computeWidth();
                    double tileHeight = tileRectangle.computeHeight();

                    const CesiumGeospatial::Projection& projection = pTileProvider->getProjection();

                    gsl::span<const CesiumGeospatial::GlobeRectangle> cutouts = overlay.getCutouts().getCutouts();

                    std::vector<unsigned char>& imageData = this->_image.image;
                    int width = this->_image.width;
                    int height = this->_image.width;

                    for (const CesiumGeospatial::GlobeRectangle& rectangle : cutouts) {
                        CesiumGeometry::Rectangle cutoutRectangle = projectRectangleSimple(projection, rectangle);
                        std::optional<CesiumGeometry::Rectangle> cutoutInTileOpt = tileRectangle.intersect(cutoutRectangle);
                        if (!cutoutInTileOpt) {
                            continue;
                        }

                        CesiumGeometry::Rectangle& cutoutInTile = cutoutInTileOpt.value();
                        double startU = (cutoutInTile.minimumX - tileRectangle.minimumX) / tileWidth;
                        double endU = (cutoutInTile.maximumX - tileRectangle.minimumX) / tileWidth;
                        double startV = (cutoutInTile.minimumY - tileRectangle.minimumY) / tileHeight;
                        double endV = (cutoutInTile.maximumY - tileRectangle.minimumY) / tileHeight;

                        // The first row in the image is at v coordinate 1.0.
                        startV = 1.0 - startV;
                        endV = 1.0 - endV;

                        std::swap(startV, endV);

                        uint32_t startPixelX = static_cast<uint32_t>(std::floor(startU * width));
                        uint32_t endPixelX = static_cast<uint32_t>(std::ceil(endU * width));
                        uint32_t startPixelY = static_cast<uint32_t>(std::floor(startV * height));
                        uint32_t endPixelY = static_cast<uint32_t>(std::ceil(endV * height));

                        for (uint32_t j = startPixelY; j < endPixelY; ++j) {
                            uint32_t rowStart = j * width * bytesPerPixel;
                            for (uint32_t i = startPixelX; i < endPixelX; ++i) {
                                uint32_t pixelStart = rowStart + i * bytesPerPixel;
                                
                                // Set alpha to 0
                                imageData[pixelStart + 3] = 0;
                            }
                        }
                    }

                    this->_pRendererResources = pTileProvider->getExternals().pPrepareRendererResources->prepareRasterInLoadThread(*this);
                    this->setState(LoadState::Loaded);
                } else {
                    this->_pRendererResources = nullptr;
                    this->setState(LoadState::Failed);
                }

                if (this->getOverlay().isBeingDestroyed()) {
                    this->getOverlay().destroySafely(nullptr);
                }

                // Now that we're done loading we can allow this tile to be destroyed.
                this->_pSelf.reset();
            });
        }

        void RasterOverlayTile::setState(LoadState newState) {
            this->_state.store(newState, std::memory_order::memory_order_release);
        }

}
