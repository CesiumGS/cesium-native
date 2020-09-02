#pragma once

#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include <memory>

namespace Cesium3DTiles {

    class Tile;

    class RasterMappedTo3DTile {
    public:
        enum class AttachmentState {
            Unattached = 0,
            Attached = 1
        };

        RasterMappedTo3DTile(
            const std::shared_ptr<RasterOverlayTile>& pRasterTile,
            const CesiumGeometry::Rectangle& textureCoordinateRectangle
        );

        RasterOverlayTile& getRasterTile() { return *this->_pRasterTile; }
        const RasterOverlayTile& getRasterTile() const { return *this->_pRasterTile; }
        const CesiumGeometry::Rectangle& getTextureCoordinateRectangle() const { return this->_textureCoordinateRectangle; }
        AttachmentState getState() const { return this->_state; }

        void attachToTile(Tile& tile);

    private:
        std::shared_ptr<RasterOverlayTile> _pRasterTile;
        CesiumGeometry::Rectangle _textureCoordinateRectangle;
        AttachmentState _state;
    };

}
