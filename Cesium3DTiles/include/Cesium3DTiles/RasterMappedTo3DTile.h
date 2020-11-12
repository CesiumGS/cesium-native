#pragma once

#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include <memory>

namespace Cesium3DTiles {

    class Tile;

    class RasterMappedTo3DTile {
    public:
        enum class AttachmentState {
            /**
             * This raster tile is not yet attached to the geometry at all.
             */
            Unattached = 0,

            /**
             * This raster tile is attached to the geometry, but it is a temporary, low-res
             * version usable while the full-res version is loading.
             * 
             */
            TemporarilyAttached = 1,

            /**
             * This raster tile is attached to the geometry.
             */
            Attached = 2
        };

        RasterMappedTo3DTile(
            const std::shared_ptr<RasterOverlayTile>& pRasterTile,
            const CesiumGeometry::Rectangle& textureCoordinateRectangle
        );

        std::shared_ptr<RasterOverlayTile>& getLoadingTile() { return this->_pLoadingTile; }
        const std::shared_ptr<RasterOverlayTile>& getLoadingTile() const { return this->_pLoadingTile; }

        std::shared_ptr<RasterOverlayTile>& getReadyTile() { return this->_pReadyTile; }
        const std::shared_ptr<RasterOverlayTile>& getReadyTile() const { return this->_pReadyTile; }

        uint32_t getTextureCoordinateID() const { return this->_textureCoordinateID; }
        void setTextureCoordinateID(uint32_t textureCoordinateID) { this->_textureCoordinateID = textureCoordinateID; }
        const CesiumGeometry::Rectangle& getTextureCoordinateRectangle() const { return this->_textureCoordinateRectangle; }
        const glm::dvec2& getTranslation() const { return this->_translation; }
        const glm::dvec2& getScale() const { return this->_scale; }
        AttachmentState getState() const { return this->_state; }

        enum class MoreDetailAvailable {
            No = 0,
            Yes = 1,
            Unknown = 2
        };

        MoreDetailAvailable update(Tile& tile);
        void detachFromTile(Tile& tile);
        
        // void attachToTile(Tile& tile);

    private:
        void computeTranslationAndScale(Tile& tile);

        std::shared_ptr<RasterOverlayTile> _pLoadingTile;
        std::shared_ptr<RasterOverlayTile> _pReadyTile;
        uint32_t _textureCoordinateID;
        CesiumGeometry::Rectangle _textureCoordinateRectangle;
        glm::dvec2 _translation;
        glm::dvec2 _scale;
        AttachmentState _state;
    };

}
