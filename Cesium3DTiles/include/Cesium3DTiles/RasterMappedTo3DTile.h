#pragma once

#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumUtility/IntrusivePointer.h"
#include <memory>

namespace Cesium3DTiles {

    class Tile;

    /**
     * @brief The result of applying a {@link RasterOverlayTile} to geometry.
     * 
     * Instances of this class are used by a {@link Tile} in order to map
     * imagery data that is given as {@link RasterOverlayTile} instances 
     * to the 2D region that is covered by the tile geometry. 
     */
    class RasterMappedTo3DTile {
    public:

        /**
         * @brief The states indicating whether the raster tile is attached to the geometry.
         */
        enum class AttachmentState {
            /**
             * @brief This raster tile is not yet attached to the geometry at all.
             */
            Unattached = 0,

            /**
             * @brief This raster tile is attached to the geometry, but it is a temporary, low-res
             * version usable while the full-res version is loading.
             */
            TemporarilyAttached = 1,

            /**
             * @brief This raster tile is attached to the geometry.
             */
            Attached = 2
        };

        /**
         * @brief Creates a new instance.
         * 
         * @param pRasterTile The {@link RasterOverlayTile} that is mapped to the geometry.
         * @param textureCoordinateRectangle The texture coordinate rectangle that indicates
         * the region that is covered by the raster overlay tile.
         */
        RasterMappedTo3DTile(
            CesiumUtility::IntrusivePointer<RasterOverlayTile> pRasterTile,
            const CesiumGeometry::Rectangle& textureCoordinateRectangle
        );

        /**
         * @brief Returns a {@link RasterOverlayTile} that serves as a placeholder while loading.
         * 
         * The caller has to check the exact state of this tile, using 
         * {@link Tile::getState}.
         * 
         * @return The placeholder tile while loading, or `nullptr`.
         */
        RasterOverlayTile* getLoadingTile() { return this->_pLoadingTile.get(); }

        /** @copydoc getLoadingTile */
        const RasterOverlayTile* getLoadingTile() const { return this->_pLoadingTile.get(); }

        /**
         * @brief Returns the {@link RasterOverlayTile} that represents the imagery data.
         * 
         * This will be `nullptr` when the tile data has not yet been loaded.
         * 
         * @return The tile, or `nullptr`.
         */
        RasterOverlayTile* getReadyTile() { return this->_pReadyTile.get(); }

        /** @copydoc getReadyTile */
        const RasterOverlayTile* getReadyTile() const { return this->_pReadyTile.get(); }

        /**
         * @brief Returns an identifier for the texture coordinates of this tile.
         * 
         * This is an unspecified identifier that is used internally by the
         * tile rendering infrastructure, to identify the texture coordinates
         * in the rendering environment.
         * 
         * @return The texture coordinate ID.
         */
        uint32_t getTextureCoordinateID() const { return this->_textureCoordinateID; }

        /**
         * @brief Sets the texture coordinate ID.
         * 
         * @see getTextureCoordinateID
         * 
         * @param textureCoordinateID The ID.
         */
        void setTextureCoordinateID(uint32_t textureCoordinateID) { this->_textureCoordinateID = textureCoordinateID; }
        
        /**
         * @brief The texture coordinate range in which the raster is applied.
         * 
         * This is part of a unit rectangle, where the rectangle
         * `(minimumX, minimumY, maximumX, maximumY)` corresponds
         * to the `(west, south, east, north)` of the tile region,
         * and each coordinate is in the range `[0,1]`.
         * 
         * @return The texture coordinate rectangle.
         */
        const CesiumGeometry::Rectangle& getTextureCoordinateRectangle() const { return this->_textureCoordinateRectangle; }
        
        /**
         * @brief Returns the translation that converts between texture coordinates and world coordinates.
         * 
         * The translation and {@link getScale} are computed from the region 
         * that is covered by the tile geometry (in the coordinates that are
         * specific for the projection that is used by the tile provider)
         * and the {@link getTextureCoordinateRectangle}.
         *
         * @returns The translation.
         */
        const glm::dvec2& getTranslation() const { return this->_translation; }

        /**
         * @brief Returns the scaling that converts between texture coordinates and world coordinates.
         * 
         * @see getTranslation
         *
         * @returns The scaling.
         */
        const glm::dvec2& getScale() const { return this->_scale; }
        
        /**
         * @brief Returns the {@link AttachmentState}.
         * 
         * This function is not supposed to be called by clients.
         */
        AttachmentState getState() const { return this->_state; }

        /**
         * @brief Tile availability states.
         * 
         * Values of this enumeration are returned by {@link update}, which
         * in turn is called by {@link Tile::update}. These values are used
         * to determine whether a leaf tile has been reached, but the 
         * associated raster tiles are not yet the most detailed ones that
         * are available.
         */
        enum class MoreDetailAvailable {

            /** @brief There are no more detailed raster tiles. */
            No = 0,

            /** @brief There are more detailed raster tiles. */
            Yes = 1,

            /** @brief It is not known whether more detailed raster tiles are available. */
            Unknown = 2
        };

        /**
         * @brief Update this tile during the update of its owner.
         * 
         * This is only supposed to be called by {@link Tile::update}. It
         * will return whether there is a more detailed version of the 
         * raster data available.
         * 
         * @param tile The owner tile.
         * @return The {@link MoreDetailAvailable} state.
         */
        MoreDetailAvailable update(Tile& tile);

        /**
         * @brief Detach the raster from the given tile.
         */
        void detachFromTile(Tile& tile);
        
        // void attachToTile(Tile& tile);

    private:
        void computeTranslationAndScale(Tile& tile);

        CesiumUtility::IntrusivePointer<RasterOverlayTile> _pLoadingTile;
        CesiumUtility::IntrusivePointer<RasterOverlayTile> _pReadyTile;
        uint32_t _textureCoordinateID;
        CesiumGeometry::Rectangle _textureCoordinateRectangle;
        glm::dvec2 _translation;
        glm::dvec2 _scale;
        AttachmentState _state;
    };

}
