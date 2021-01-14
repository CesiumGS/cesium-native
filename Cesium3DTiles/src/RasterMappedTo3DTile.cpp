#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "TileUtilities.h"

namespace Cesium3DTiles {

    RasterMappedTo3DTile::RasterMappedTo3DTile(
        CesiumUtility::IntrusivePointer<RasterOverlayTile> pRasterTile,
        const CesiumGeometry::Rectangle& textureCoordinateRectangle
    ) :
        _pLoadingTile(pRasterTile),
        _pReadyTile(nullptr),
        _textureCoordinateID(0),
        _textureCoordinateRectangle(textureCoordinateRectangle),
        _translation(0.0, 0.0),
        _scale(1.0, 1.0),
        _state(AttachmentState::Unattached)
    {
    }

    RasterMappedTo3DTile::MoreDetailAvailable RasterMappedTo3DTile::update(Tile& tile) {
        if (this->getState() == AttachmentState::Attached) {
            return this->_pReadyTile && this->_pReadyTile->getID().level < this->_pReadyTile->getOverlay().getTileProvider()->getMaximumLevel()
                ? MoreDetailAvailable::Yes
                : MoreDetailAvailable::No;
        }

        // If the loading tile has failed, try its parent.
        while (this->_pLoadingTile && this->_pLoadingTile->getState() == RasterOverlayTile::LoadState::Failed && this->_pLoadingTile->getID().level > 0) {
            CesiumGeometry::QuadtreeTileID thisID = this->_pLoadingTile->getID();
            CesiumGeometry::QuadtreeTileID parentID(thisID.level - 1, thisID.x >> 1, thisID.y >> 1);
            this->_pLoadingTile = this->_pLoadingTile->getOverlay().getTileProvider()->getTile(parentID);
        }

        // If the loading tile is now ready, make it the ready tile.
        if (this->_pLoadingTile && this->_pLoadingTile->getState() >= RasterOverlayTile::LoadState::Loaded) {
            // Unattach the old tile
            if (this->_pReadyTile && this->getState() != AttachmentState::Unattached) {
                TilesetExternals& externals = tile.getTileset()->getExternals();
                externals.pPrepareRendererResources->detachRasterInMainThread(
                    tile,
                    this->getTextureCoordinateID(),
                    *this->_pReadyTile,
                    this->_pReadyTile->getRendererResources(),
                    this->getTextureCoordinateRectangle()
                );
                this->_state = AttachmentState::Unattached;
            }

            // Mark the loading tile read.
            this->_pReadyTile = this->_pLoadingTile;
            this->_pLoadingTile = nullptr;

            // Compute the translation and scale for the new tile.
            this->computeTranslationAndScale(tile);
        }

        // Find the closest ready ancestor tile.
        if (this->_pLoadingTile) {
            RasterOverlayTileProvider& tileProvider = *this->_pLoadingTile->getOverlay().getTileProvider();

            CesiumUtility::IntrusivePointer<RasterOverlayTile> pCandidate;
            CesiumGeometry::QuadtreeTileID id = this->_pLoadingTile->getID();
            while (id.level > 0) {
                --id.level;
                id.x >>= 1;
                id.y >>= 1;

                pCandidate = tileProvider.getTileWithoutRequesting(id);
                if (pCandidate && pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded) {
                    break;
                }
            }

            if (pCandidate && pCandidate->getState() >= RasterOverlayTile::LoadState::Loaded && this->_pReadyTile != pCandidate) {
                if (this->getState() != AttachmentState::Unattached) {
                    TilesetExternals& externals = tile.getTileset()->getExternals();
                    externals.pPrepareRendererResources->detachRasterInMainThread(
                        tile,
                        this->getTextureCoordinateID(),
                        *this->_pReadyTile,
                        this->_pReadyTile->getRendererResources(),
                        this->getTextureCoordinateRectangle()
                    );
                    this->_state = AttachmentState::Unattached;
                }

                this->_pReadyTile = pCandidate;

                // Compute the translation and scale for the new tile.
                this->computeTranslationAndScale(tile);
            }
        }

        // Attach the ready tile if it's not already attached.
        if (this->_pReadyTile && this->getState() == RasterMappedTo3DTile::AttachmentState::Unattached) {
            this->_pReadyTile->loadInMainThread();

            TilesetExternals& externals = tile.getTileset()->getExternals();
            externals.pPrepareRendererResources->attachRasterInMainThread(
                tile,
                this->getTextureCoordinateID(),
                *this->_pReadyTile,
                this->_pReadyTile->getRendererResources(),
                this->getTextureCoordinateRectangle(),
                this->getTranslation(),
                this->getScale()
            );

            this->_state = this->_pLoadingTile ? AttachmentState::TemporarilyAttached : AttachmentState::Attached;
        }

        // TODO: check more precise raster overlay tile availability, rather than just max level?
        if (this->_pLoadingTile) {
            return MoreDetailAvailable::Unknown;
        } else {
            return this->_pReadyTile && this->_pReadyTile->getID().level < this->_pReadyTile->getOverlay().getTileProvider()->getMaximumLevel()
                ? MoreDetailAvailable::Yes
                : MoreDetailAvailable::No;
        }
    }

    void RasterMappedTo3DTile::detachFromTile(Tile& tile) noexcept {
        if (this->getState() == AttachmentState::Unattached) {
            return;
        }

        if (!this->_pReadyTile) {
            return;
        }

        TilesetExternals& externals = tile.getTileset()->getExternals();
        externals.pPrepareRendererResources->detachRasterInMainThread(
            tile,
            this->getTextureCoordinateID(),
            *this->_pReadyTile,
            this->_pReadyTile->getRendererResources(),
            this->getTextureCoordinateRectangle()
        );

        this->_state = AttachmentState::Unattached;
    }

    void RasterMappedTo3DTile::computeTranslationAndScale(Tile& tile) {
        if (!this->_pReadyTile) {
            return;
        }

        const CesiumGeospatial::GlobeRectangle* pRectangle = Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
        if (!pRectangle) {
            return;
        }

        RasterOverlayTileProvider& tileProvider = *this->_pReadyTile->getOverlay().getTileProvider();
        CesiumGeometry::Rectangle geometryRectangle = projectRectangleSimple(tileProvider.getProjection(), *pRectangle);
        CesiumGeometry::Rectangle imageryRectangle = tileProvider.getTilingScheme().tileToRectangle(this->_pReadyTile->getID());

        double terrainWidth = geometryRectangle.computeWidth();
        double terrainHeight = geometryRectangle.computeHeight();

        double scaleX = terrainWidth / imageryRectangle.computeWidth();
        double scaleY = terrainHeight / imageryRectangle.computeHeight();
        this->_translation = glm::dvec2(
            (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) / terrainWidth,
            (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) / terrainHeight
        );
        this->_scale = glm::dvec2(scaleX, scaleY);
    }

}
