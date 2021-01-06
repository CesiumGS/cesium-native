#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/ITaskProcessor.h"

using namespace CesiumAsync;

namespace Cesium3DTiles {

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlay& overlay
        ) noexcept :
            _pOverlay(&overlay),
            _tileID(0, 0, 0),
            _tileCredits(),
            _state(LoadState::Placeholder),
            _image(),
            _pRendererResources(nullptr),
            _references(0)
        {
        }

        RasterOverlayTile::RasterOverlayTile(
            RasterOverlay& overlay,
            const CesiumGeometry::QuadtreeTileID& tileID,
            const std::vector<Credit>& tileCredits,
            Future<std::unique_ptr<IAssetRequest>>&& imageRequest
        ) :
            _pOverlay(&overlay),
            _tileID(tileID),
            _tileCredits(tileCredits),
            _state(LoadState::Unloaded),
            _image(),
            _pRendererResources(nullptr),
            _references(0)
        {
            struct LoadResult {
                LoadResult() = default;
                LoadResult(LoadState state_) :
                    state(state_),
                    image(),
                    warnings(),
                    errors(),
                    pRendererResources(nullptr)
                {}

                LoadState state;
                tinygltf::Image image;
                std::string warnings;
                std::string errors;
                void* pRendererResources;
            };

            this->addReference();
            this->setState(LoadState::Loading);

            RasterOverlayTileProvider* pTileProvider = overlay.getTileProvider();

            std::move(imageRequest).thenInWorkerThread([
                tileRectangle = pTileProvider->getTilingScheme().tileToRectangle(this->getID()),
                projection = pTileProvider->getProjection(),
                cutoutsCollection = overlay.getCutouts(),
                pPrepareRendererResources = pTileProvider->getPrepareRendererResources()
            ](
                std::unique_ptr<IAssetRequest> pRequest
            ) {
                const IAssetResponse* pResponse = pRequest->response();
                if (pResponse == nullptr) {
                    return LoadResult(LoadState::Failed);
                }

                if (pResponse->data().size() == 0) {
                    return LoadResult(LoadState::Failed);
                }

                LoadResult result;

                gsl::span<const uint8_t> data = pResponse->data();
                bool success = tinygltf::LoadImageData(&result.image, 0, &result.errors, &result.warnings, 0, 0, data.data(), static_cast<int>(data.size()), nullptr);

                const int bytesPerPixel = 4;
                if (success && result.image.image.size() >= static_cast<size_t>(result.image.width * result.image.height * bytesPerPixel)) {
                    double tileWidth = tileRectangle.computeWidth();
                    double tileHeight = tileRectangle.computeHeight();

                    gsl::span<const CesiumGeospatial::GlobeRectangle> cutouts = cutoutsCollection.getCutouts();

                    std::vector<unsigned char>& imageData = result.image.image;
                    int width = result.image.width; 
                    int height = result.image.width;

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
                            uint32_t rowStart = j * static_cast<uint32_t>(width) * static_cast<uint32_t>(bytesPerPixel);
                            for (uint32_t i = startPixelX; i < endPixelX; ++i) {
                                uint32_t pixelStart = rowStart + i * bytesPerPixel;
                                
                                // Set alpha to 0
                                imageData[pixelStart + 3] = 0;
                            }
                        }
                    }

                    result.pRendererResources = pPrepareRendererResources->prepareRasterInLoadThread(result.image);

                    result.state = LoadState::Loaded;
                } else {
                    result.pRendererResources = nullptr;
                    result.state = LoadState::Failed;
                }

                return result;
            }).thenInMainThread([this](LoadResult&& result) {
                result.pRendererResources = result.pRendererResources;
                this->_image = std::move(result.image);
                this->setState(result.state);

                assert(this->_pOverlay != nullptr);
                assert(this->_pOverlay->getTileProvider() != nullptr);
                this->_pOverlay->getTileProvider()->notifyTileLoaded(this);

                this->releaseReference();
            }).catchInMainThread([this](const std::exception& /*e*/) {
                this->setState(LoadState::Failed);
                this->_pOverlay->getTileProvider()->notifyTileLoaded(this);
                this->releaseReference();
            });
        }

        RasterOverlayTile::~RasterOverlayTile() {
            RasterOverlayTileProvider* pTileProvider = this->_pOverlay->getTileProvider();
            if (pTileProvider) {
                const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources = pTileProvider->getPrepareRendererResources();

                if (pPrepareRendererResources) {
                    void* pLoadThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? nullptr : this->_pRendererResources;
                    void* pMainThreadResult = this->getState() == RasterOverlayTile::LoadState::Done ? this->_pRendererResources : nullptr;

                    pPrepareRendererResources->freeRaster(*this, pLoadThreadResult, pMainThreadResult);
                }
            }
        }

        void RasterOverlayTile::loadInMainThread() {
            if (this->getState() != RasterOverlayTile::LoadState::Loaded) {
                return;
            }

            // Do the final main thread raster loading
            RasterOverlayTileProvider* pTileProvider = this->_pOverlay->getTileProvider();
            this->_pRendererResources = pTileProvider->getPrepareRendererResources()->prepareRasterInMainThread(*this, this->_pRendererResources);

            this->setState(LoadState::Done);
        }

        void RasterOverlayTile::addReference() noexcept {
            ++this->_references;
        }

        void RasterOverlayTile::releaseReference() noexcept {
            assert(this->_references > 0);
            uint32_t references = --this->_references;
            if (references == 0) {
                assert(this->_pOverlay != nullptr);
                assert(this->_pOverlay->getTileProvider() != nullptr);
                this->_pOverlay->getTileProvider()->removeTile(this);
            }
        }

        void RasterOverlayTile::setState(LoadState newState) {
            this->_state.store(newState, std::memory_order::memory_order_release);
        }

}
