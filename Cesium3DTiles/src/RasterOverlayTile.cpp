#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumGltf/Reader.h"
#include "CesiumUtility/joinToString.h"

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
                CesiumGltf::ImageCesium image;
                std::vector<std::string> warnings;
                std::vector<std::string> errors;
                void* pRendererResources;
            };

            this->addReference();
            this->setState(LoadState::Loading);

            RasterOverlayTileProvider* pTileProvider = overlay.getTileProvider();

            std::move(imageRequest).thenInWorkerThread([
                tileRectangle = pTileProvider->getTilingScheme().tileToRectangle(this->getID()),
                projection = pTileProvider->getProjection(),
                cutoutsCollection = overlay.getCutouts(),
                pPrepareRendererResources = pTileProvider->getPrepareRendererResources(),
                pLogger = pTileProvider->getLogger()
            ](
                std::unique_ptr<IAssetRequest> pRequest
            ) {
                IAssetResponse* pResponse = pRequest->response();
                if (pResponse == nullptr) {
                    return LoadResult(LoadState::Failed);
                }

                if (pResponse->data().size() == 0) {
                    return LoadResult(LoadState::Failed);
                }

                gsl::span<const uint8_t> data = pResponse->data();
                CesiumGltf::ImageReaderResult loadedImage = CesiumGltf::readImage(data);

                if (!loadedImage.image.has_value()) {
                    SPDLOG_LOGGER_ERROR(pLogger, "Failed to load image:\n- {}", CesiumUtility::joinToString(loadedImage.errors, "\n- "));

                    LoadResult result;
                    result.pRendererResources = nullptr;
                    result.state = LoadState::Failed;
                    return result;
                }

                if (!loadedImage.warnings.empty()) {
                    SPDLOG_LOGGER_WARN(pLogger, "Warnings while loading image:\n- {}", CesiumUtility::joinToString(loadedImage.warnings, "\n- "));
                }

                CesiumGltf::ImageCesium& image = loadedImage.image.value();

                int32_t bytesPerPixel = image.channels * image.bytesPerChannel;

                if (image.pixelData.size() >= static_cast<size_t>(image.width * image.height * bytesPerPixel)) {
                    double tileWidth = tileRectangle.computeWidth();
                    double tileHeight = tileRectangle.computeHeight();

                    gsl::span<const CesiumGeospatial::GlobeRectangle> cutouts = cutoutsCollection.getCutouts();

                    std::vector<uint8_t>& imageData = image.pixelData;
                    int width = image.width; 
                    int height = image.height;

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

                        int32_t startPixelX = static_cast<int32_t>(std::floor(startU * width));
                        int32_t endPixelX = static_cast<int32_t>(std::ceil(endU * width));
                        int32_t startPixelY = static_cast<int32_t>(std::floor(startV * height));
                        int32_t endPixelY = static_cast<int32_t>(std::ceil(endV * height));

                        for (int32_t j = startPixelY; j < endPixelY; ++j) {
                            int32_t rowStart = j * width * bytesPerPixel;
                            for (int32_t i = startPixelX; i < endPixelX; ++i) {
                                int32_t pixelStart = rowStart + i * bytesPerPixel;
                                
                                // Set alpha to 0
                                imageData[size_t(pixelStart + 3)] = 0;
                            }
                        }
                    }

                    void* pRendererResources = pPrepareRendererResources->prepareRasterInLoadThread(image);

                    LoadResult result;
                    result.state = LoadState::Loaded;
                    result.image = std::move(image);
                    result.pRendererResources = pRendererResources;
                    result.errors = std::move(loadedImage.errors);
                    result.warnings = std::move(loadedImage.warnings);
                    return result;
                } else {
                    LoadResult result;
                    result.pRendererResources = nullptr;
                    result.state = LoadState::Failed;
                    return result;
                }
            }).thenInMainThread([this](LoadResult&& result) {
                this->_pRendererResources = result.pRendererResources;
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
