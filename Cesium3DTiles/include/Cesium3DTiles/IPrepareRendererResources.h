#pragma once

#include "Cesium3DTiles/Library.h"
#include <glm/vec2.hpp>
#include <gsl/span>

namespace CesiumGeometry {
    struct Rectangle;
}

namespace Cesium3DTiles {

    class IAssetAccessor;
    class Tile;
    class RasterOverlayTile;

    /**
     * @brief When implemented for a rendering engine, allows renderer resources to be created and destroyed under the control of a {@link Tileset}.
     * 
     * It is not supposed to be used directly by clients. It is implemented
     * for specific rendering engines to provide an infrastructure for preparing the
     * data of a {@link Tile} so that it can be used for rendering.
     * 
     * Instances of this class are associated with a {@link Tileset}, in the
     * {@link TilesetExternals} structure that can be obtained 
     * via {@link Tileset::getExternals}.
     */
    class CESIUM3DTILES_API IPrepareRendererResources {
    public:
        virtual ~IPrepareRendererResources() = default;

        /**
         * @brief Prepares renderer resources for the given tile. 
         *
         * This method is invoked in the load thread, and it may not modify the tile.
         * 
         * @param tile The tile to prepare.
         * @returns Arbitrary data representing the result of the load process. This data is
         * passed to {@link prepareInMainThread} as the `pLoadThreadResult` parameter.
         */
        virtual void* prepareInLoadThread(const Tile& tile) = 0;

        /**
         * @brief Further prepares renderer resources. 
         * 
         * This is called after {@link prepareInLoadThread}, and unlike that method, 
         * this one is called from the same thread that called {@link Tileset::updateView}.
         * 
         * @param tile The tile to prepare.
         * @param pLoadThreadResult The value returned from {@link prepareInLoadThread}.
         * @returns Arbitrary data representing the result of the load process. 
         * Note that the value returned by {@link prepareInLoadThread} will _not_ be 
         * automatically preserved and passed to {@link free}. If you need to free that 
         * value, do it in this method before returning. If you need that value later, 
         * add it to the object returned from this method.
         */
        virtual void* prepareInMainThread(Tile& tile, void* pLoadThreadResult) = 0;

        /**
         * @brief Frees previously-prepared renderer resources. 
         *
         * This method is always called from the thread that called {@link Tileset::updateView}
         * or deleted the tileset.
         * 
         * @param tile The tile for which to free renderer resources.
         * @param pLoadThreadResult The result returned by {@link prepareInLoadThread}. 
         * If {@link prepareInMainThread} has already been called, this parameter will be `nullptr`.
         * @param pMainThreadResult The result returned by {@link prepareInMainThread}. 
         * If {@link prepareInMainThread} has not yet been called, this parameter will be `nullptr`.
         */
        virtual void free(Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) = 0;

        /**
         * @brief Prepares a raster overlay tile. 
         *
         * This method is invoked in the load thread, and it may not modify the tile.
         * 
         * @param rasterTile The raster tile to prepare.
         * @returns Arbitrary data representing the result of the load process. This data is
         * passed to {@link prepareRasterInMainThread} as the `pLoadThreadResult` parameter.
         */
        virtual void* prepareRasterInLoadThread(const RasterOverlayTile& rasterTile) = 0;

        /**
         * @brief Further preprares a raster overlay tile. 
         * 
         * This is called after {@link prepareRasterInLoadThread}, and unlike that method, 
         * this one is called from the same thread that called {@link Tileset::updateView}.
         * 
         * @param rasterTile The raster tile to prepare.
         * @param pLoadThreadResult The value returned from {@link prepareInLoadThread}.
         * @returns Arbitrary data representing the result of the load process. Note that the 
         * value returned by {@link prepareRasterInLoadThread} will _not_ be automatically 
         * preserved and passed to {@link free}. If you need to free that value, do it in this 
         * method before returning. If you need that value later, add it to the object returned 
         * from this method.
         */
        virtual void* prepareRasterInMainThread(const RasterOverlayTile& rasterTile, void* pLoadThreadResult) = 0;

        /**
         * @brief Frees previously-prepared renderer resources for a raster tile. 
         * 
         * This method is always called from the thread that called {@link Tileset::updateView} 
         * or deleted the tileset.
         * 
         * @param rasterTile The tile for which to free renderer resources.
         * @param pLoadThreadResult The result returned by {@link prepareRasterInLoadThread}. 
         * If {@link prepareRasterInMainThread} has already been called, this parameter will 
         * be `nullptr`.
         * @param pMainThreadResult The result returned by {@link prepareRasterInMainThread}. 
         * If {@link prepareRasterInMainThread} has not yet been called, this parameter will 
         * be `nullptr`.
         */
        virtual void freeRaster(const RasterOverlayTile& rasterTile, void* pLoadThreadResult, void* pMainThreadResult) = 0;

        /**
         * @brief Attaches a raster overlay tile to a geometry tile.
         * 
         * @param tile The geometry tile.
         * @param overlayTextureCoordinateID The ID of the overlay texture coordinate set to use. The texture coordinates have
         *        previously been added by a call to {@link addRasterOverlayTextureCoordinates}.
         * @param rasterTile The raster overlay tile to add. The raster tile will have been previously prepared with a call to
         *        {@link prepareRasterInLoadThread} followed by {@link prepareRasterInMainThread}.
         * @param pMainThreadRendererResources The renderer resources for this raster tile, as created and returned by
         *        {@link prepareRasterInMainThread}.
         * @param textureCoordinateRectangle Defines the range of texture coordinates in which this raster tile should be applied, in
         *        the order west, south, east north. Each coordinate is in the range 0.0 (southwest corner) to 1.0 (northeast corner).
         * @param translation The translation to apply to the texture coordinates identified by `overlayTextureCoordinateID`. The
         *        texture coordinates to use to sample the raster image are computed as `overlayTextureCoordinates * scale + translation`.
         * @param scale The scale to apply to the texture coordinates identified by `overlayTextureCoordinateID`. The
         *        texture coordinates to use to sample the raster image are computed as `overlayTextureCoordinates * scale + translation`.
         */
        virtual void attachRasterInMainThread(
            const Tile& tile,
            uint32_t overlayTextureCoordinateID,
            const RasterOverlayTile& rasterTile,
            void* pMainThreadRendererResources,
            const CesiumGeometry::Rectangle& textureCoordinateRectangle,
            const glm::dvec2& translation,
            const glm::dvec2& scale
        ) = 0;

        /**
         * @brief Detaches a raster overlay tile from a geometry tile.
         * 
         * @param tile The geometry tile.
         * @param overlayTextureCoordinateID The ID of the overlay texture coordinate set to which the raster tile was previously attached.
         * @param rasterTile The raster overlay tile to remove.
         * @param pMainThreadRendererResources The renderer resources for this raster tile, as created and returned by
         *        {@link prepareRasterInMainThread}.
         * @param textureCoordinateRectangle Defines the range of texture coordinates in which this raster tile should be applied, in
         *        the order west, south, east north. Each coordinate is in the range 0.0 (southwest corner) to 1.0 (northeast corner).
         */
        virtual void detachRasterInMainThread(
            const Tile& tile,
            uint32_t overlayTextureCoordinateID,
            const RasterOverlayTile& rasterTile,
            void* pMainThreadRendererResources,
            const CesiumGeometry::Rectangle& textureCoordinateRectangle
        ) = 0;
    };

}
