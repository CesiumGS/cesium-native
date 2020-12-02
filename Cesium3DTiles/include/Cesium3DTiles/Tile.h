#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileSelectionState.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumUtility/DoublyLinkedList.h"
#include <atomic>
#include <glm/mat4x4.hpp>
#include <gsl/span>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {
    class Tileset;
    class TileContent;
    struct TileContentLoadResult;

    /**
     * @brief A tile in a {@link Tileset}.
     * 
     * The tiles of a tileset form a hierarchy, where each tile may contain 
     * renderable content, and each tile has an associated bounding volume.
     * 
     * The actual hierarchy is represented with the {@link Tile::getParent}
     * and {@link Tile::getChildren} functions.
     * 
     * The renderable content is provided as a {@link TileContentLoadResult}
     * from the {@link Tile::getContent} function. The {@link Tile::getGeometricError}
     * function returns the geometric error of the representation of the renderable 
     * content of a tile.
     * 
     * The {@link BoundingVolume} is given by the {@link Tile::getBoundingVolume}
     * function. This bounding volume encloses the renderable content of the 
     * tile itself, as well as the renderable content of all children, yielding 
     * a spatially coherent hierarchy of bounding volumes.
     * 
     * The bounding volume of the content of an individual tile is given 
     * by the {@link Tile::getContentBoundingVolume} function.
     * 
     */
    class CESIUM3DTILES_API Tile {
    public:
        /**
         * The current state of this tile in the loading process.
         */
        enum class LoadState {
            /**
             * @brief This tile is in the process of being destroyed. 
             *
             * Any pointers to it will soon be invalid.
             */
            Destroying = -3,

            /**
             * @brief Something went wrong while loading this tile and it will not be retried.
             */
            Failed = -2,

            /**
             * @brief Something went wrong while loading this tile, but it may be a temporary problem.
             */
            FailedTemporarily = -1,

            /**
             * @brief The tile is not yet loaded at all, beyond the metadata in tileset.json.
             */
            Unloaded = 0,

            /**
             * @brief The tile content is currently being loaded. 
             *
             * Note that while a tile is in this state, its {@link Tile::getContent},
             * {@link Tile::getState}, and {@link Tile::setState} methods may be 
             * called from the load thread.
             */
            ContentLoading = 1,

            /**
             * @brief The tile content has finished loading.
             */
            ContentLoaded = 2,

            /**
             * @brief The tile is completely done loading.
             */
            Done = 3
        };

        /**
         * @brief Default constructor for an empty, uninitialized tile.
         */
        Tile();

        /**
         * @brief Default destructor, which clears all resources associated with this tile.
         */
        ~Tile();

        /**
         * @brief Copy constructor.
         *
         * @param rhs The other instance.
         */
        Tile(Tile& rhs) noexcept = delete;

        /**
         * @brief Move constructor.
         * 
         * @param rhs The other instance.
         */
        Tile(Tile&& rhs) noexcept;

        /**
         * @brief Move assignment operator.
         *
         * @param rhs The other instance.
         */
        Tile& operator=(Tile&& rhs) noexcept;

        /**
         * @brief Returns the {@link Tileset} to which this tile belongs.
         */
        Tileset* getTileset() noexcept { return this->_pContext->pTileset; }

        /** @copydoc Tile::getTileset() */
        const Tileset* getTileset() const { return this->_pContext->pTileset; }
        
        /**
         * @brief Returns the {@link TileContext} of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @return The tile context.
         */
        TileContext* getContext() { return this->_pContext; }

        /** @copydoc Tile::getContext() */
        const TileContext* getContext() const { return this->_pContext; }

        /**
         * @brief Set the {@link TileContext} of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @param pContext The tile context.
         */
        void setContext(TileContext* pContext) { this->_pContext = pContext; }

        /**
         * @brief Returns the parent of this tile in the tile hierarchy.
         * 
         * This will be the `nullptr` if this is the root tile.
         * 
         * @return The parent.
         */
        Tile* getParent() { return this->_pParent; }

        /** @copydoc Tile::getParent() */
        const Tile* getParent() const { return this->_pParent; }

        /**
         * @brief Set the parent of this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param pParent The parent tile .
         */
        void setParent(Tile* pParent) { this->_pParent = pParent; }

        /**
         * @brief Returns a *view* on the children of this tile.
         * 
         * The returned span will become invalid when this tile is destroyed.
         * 
         * @return The children of this tile.
         */
        gsl::span<Tile> getChildren() noexcept { return gsl::span<Tile>(this->_children); }

        /** @copydoc Tile::getChildren() */
        gsl::span<const Tile> getChildren() const { return gsl::span<const Tile>(this->_children); }

        /**
         * @brief Allocates space for the given number of child tiles.
         * 
         * This function is not supposed to be called by clients.
         * 
         * @param count The number of child tiles.
         * @throws `std::runtime_error` if this tile already has children.
         */
        void createChildTiles(size_t count);

        /**
         * @brief Assigns the given child tiles to this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param children The child tiles.
         * @throws `std::runtime_error` if this tile already has children.
         */
        void createChildTiles(std::vector<Tile>&& children);

        /**
         * @brief Returns the {@link BoundingVolume} of this tile.
         * 
         * This is a bounding volume that encloses the content of this tile,
         * as well as the content of all child tiles. 
         * 
         * @see Tile::getContentBoundingVolume
         * 
         * @return The bounding volume.
         */
        const BoundingVolume& getBoundingVolume() const { return this->_boundingVolume; }

        /**
         * @brief Set the {@link BoundingVolume} of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @param value The bounding volume.
         */
        void setBoundingVolume(const BoundingVolume& value) { this->_boundingVolume = value; }

        /**
         * @brief Returns the viewer request volume of this tile.
         * 
         * The viewer request volume is an optional {@link BoundingVolume} that
         * may be associated with a tile. It allows controlling the rendering 
         * process of the tile content: If the viewer request volume is present,
         * then the content of the tile will only be rendered when the viewer
         * (i.e. the camera position) is inside the viewer request volume.
         * 
         * @return The viewer request volume, or an empty optional.
         */
        const std::optional<BoundingVolume>& getViewerRequestVolume() const { return this->_viewerRequestVolume; }

        /**
         * @brief Set the viewer request volume of this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param value The viewer request volume.
         */
        void setViewerRequestVolume(const std::optional<BoundingVolume>& value) { this->_viewerRequestVolume = value; }

        /**
         * @brief Returns the geometric error of this tile.
         *
         * This is the error, in meters, introduced if this tile is rendered and its children 
         * are not. This is used to compute screen space error, i.e., the error measured in pixels.
         * 
         * @return The geometric error of this tile, in meters.
         */
        double getGeometricError() const { return this->_geometricError; }

        /**
         * @brief Set the geometric error of the contents of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @param value The geometric error, in meters.
         */
        void setGeometricError(double value) { this->_geometricError = value; }

        /**
         * @brief The refinement strategy of this tile.
         * 
         * Returns the {@link TileRefine} value that indicates the refinement strategy
         * for this tile. This is `Add` when the content of the 
         * child tiles is *added* to the content of this tile during refinement, and
         * `Replace` when the content of the child tiles *replaces*
         * the content of this tile during refinement. 
         * 
         * @return The refinement strategy.
         */
        TileRefine getRefine() const { return this->_refine; }

        /**
         * @brief Set the refinement strategy of this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param value The refinement strategy.
         */
        void setRefine(TileRefine value) { this->_refine = value; }

        /**
        * @brief Gets the transformation matrix for this tile. 
        *
        * This matrix does _not_ need to be multiplied with the tile's parent's transform 
        * as this has already been done.
        * 
        * @return The transform matrix.
        */
        const glm::dmat4x4& getTransform() const { return this->_transform; }

        /**
         * @brief Set the transformation matrix for this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param value The transform matrix.
         */
        void setTransform(const glm::dmat4x4& value) { this->_transform = value; }

        /**
         * @brief Returns the {@link TileID} of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @return The tile ID.
         */
        const TileID& getTileID() const noexcept { return this->_id; }

        /**
         * @brief Set the {@link TileID} of this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @param id The tile ID.
         */
        void setTileID(const TileID& id);

        /**
         * @brief Returns the {@link BoundingVolume} of the renderable content of this tile.
         * 
         * The content bounding volume is a bounding volume that tightly fits only the
         * renderable content of the tile. This enables tighter view frustum culling, 
         * making it possible to exclude from rendering any content not in the view
         * frustum.
         * 
         * @see Tile::getBoundingVolume
         */
        const std::optional<BoundingVolume>& getContentBoundingVolume() const { return this->_contentBoundingVolume; }

        /**
         * @brief Set the {@link BoundingVolume} of the renderable content of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @param value The content bounding volume
         */
        void setContentBoundingVolume(const std::optional<BoundingVolume>& value) { this->_contentBoundingVolume = value; }

        /**
         * @brief Returns the {@link TileContentLoadResult} for the content of this tile.
         * 
         * This will be a `nullptr` if the content of this tile has not yet been loaded,
         * as indicated by the indicated by the {@link Tile::getState} of 
         * this tile not being {@link Tile::LoadState::ContentLoaded}.
         * 
         * @return The tile content load result, or `nullptr` if no content is loaded
         */
        TileContentLoadResult* getContent() { return this->_pContent.get(); }

        /** @copydoc Tile::getContent() */
        const TileContentLoadResult* getContent() const { return this->_pContent.get(); }

        /** 
         * @brief Returns internal resources required for rendering this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @return The renderer resources.
         */
        void* getRendererResources() const { return this->_pRendererResources; }

        /**
         * @brief Returns the {@link LoadState} of this tile.
         */
        LoadState getState() const noexcept { return this->_state.load(std::memory_order::memory_order_acquire); }

        /**
         * @brief Returns the {@link TileSelectionState} of this tile.
         *
         * This function is not supposed to be called by clients.
         *
         * @return The last selection state
         */
        TileSelectionState& getLastSelectionState() { return this->_lastSelectionState; }

        /** @copydoc Tile::getLastSelectionState() */
        const TileSelectionState& getLastSelectionState() const { return this->_lastSelectionState; }

        /**
         * @brief Set the {@link TileSelectionState} of this tile.
         * 
         * This function is not supposed to be called by clients.
         *
         * @param newState The new stace
         */
        void setLastSelectionState(const TileSelectionState& newState) { this->_lastSelectionState = newState; }

        /**
         * @brief Returns the raster overlay tiles that have been mapped to this tile.
         */
        std::vector<RasterMappedTo3DTile>& getMappedRasterTiles() { return this->_rasterTiles; }

        /** @copydoc Tile::getMappedRasterTiles() */
        const std::vector<RasterMappedTo3DTile>& getMappedRasterTiles() const { return this->_rasterTiles; }

        /**
         * @brief Determines if this tile is currently renderable.
         */
        bool isRenderable() const;

        /**
         * @brief Trigger the process of loading the {@link Tile::getContent}.
         * 
         * This function is not supposed to be called by clients.
         * 
         * If this tile is not in its initial state (indicated by the {@link Tile::getState} of 
         * this tile being *not* {@link Tile::LoadState::Unloaded}), then nothing will be done.
         * 
         * Otherwise, the tile will go into the {@link Tile::LoadState::ContentLoading}
         * state, and the request for loading the tile content will be sent out. The 
         * function will then return, and the response of the request will be received 
         * asynchronously. Depending on the type of the tile and the response, the tile 
         * will eventually go into the {@link Tile::LoadState::ContentLoaded} state, 
         * and the {@link Tile::getContent} will be available.
         */
        void loadContent();

        /**
         * @brief Frees all resources that have been allocated for the {@link Tile::getContent}.
         * 
         * This function is not supposed to be called by clients.
         *
         * If the operation for loading the tile content is currently in progress, as indicated by
         * the {@link Tile::getState} of this tile being {@link Tile::LoadState::ContentLoading}), 
         * then nothing will be done, and `false` will be returned.
         * 
         * Otherwise, the resources that have been allocated for the tile content will
         * be freed.
         * 
         * @return Whether the content was unloaded.
         */
        bool unloadContent() noexcept;

        /**
         * @brief Gives this tile a chance to update itself each render frame.
         * 
         * @param previousFrameNumber The number of the previous render frame.
         * @param currentFrameNumber The number of the current render frame.
         */
        void update(uint32_t previousFrameNumber, uint32_t currentFrameNumber);

        /**
         * @brief Marks the tile as permanently failing to load.
         * 
         * This function is not supposed to be called by clients.
         * 
         * Moves the tile from the `FailedTemporarily` state to the `Failed` state.
         * If the tile is not in the `FailedTemporarily` state, this method does nothing.
         */
        void markPermanentlyFailed();

        /**
         * @brief Determines the number of bytes in this tile's geometry and texture data.
         */
        size_t computeByteSize() const;

    private:

        /**
         * @brief Set the {@link LoadState} of this tile.
         */
        void setState(LoadState value) noexcept;

        /**
         * @brief Generates texture coordiantes for the raster overlays of the content of this tile.
         *
         * This will extend the accessors of the glTF model of the content of this
         * tile with accessors that contain the texture coordinate sets for different
         * projections. Further details are not specified here.
         *
         * @return The bounding region
         */
        static std::optional<CesiumGeospatial::BoundingRegion> generateTextureCoordinates(tinygltf::Model& model, const BoundingVolume& boundingVolume, const std::vector<CesiumGeospatial::Projection>& projections);

        /**
         * @brief Upsample the parent of this tile.
         *
         * This method should only be called when this tile's parent is already loaded.
         */
        void upsampleParent(std::vector<CesiumGeospatial::Projection>&& projections);

        // Position in bounding-volume hierarchy.
        TileContext* _pContext;
        Tile* _pParent;
        std::vector<Tile> _children;

        // Properties from tileset.json.
        // These are immutable after the tile leaves TileState::Unloaded.
        BoundingVolume _boundingVolume;
        std::optional<BoundingVolume> _viewerRequestVolume;
        double _geometricError;
        TileRefine _refine;
        glm::dmat4x4 _transform;

        TileID _id;
        std::optional<BoundingVolume> _contentBoundingVolume;

        // Load state and data.
        std::atomic<LoadState> _state;
        std::unique_ptr<TileContentLoadResult> _pContent;
        void* _pRendererResources;

        // Selection state
        TileSelectionState _lastSelectionState;

        // Overlays
        std::vector<RasterMappedTo3DTile> _rasterTiles;

        CesiumUtility::DoublyLinkedListPointers<Tile> _loadedTilesLinks;

    public:

        /**
         * @brief A {@link CesiumUtility::DoublyLinkedList} for tile objects.
         */
        typedef CesiumUtility::DoublyLinkedList<Tile, &Tile::_loadedTilesLinks> LoadedLinkedList;
    };

}
