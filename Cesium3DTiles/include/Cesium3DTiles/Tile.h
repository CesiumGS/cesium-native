#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileSelectionState.h"
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
    class RasterOverlayTileProvider;
    struct TileContentLoadResult;

    /**
     * @brief A tile in a {@link Tileset}.
     * 
     * The tiles of a tileset form a hierarchy, where each tile may contain 
     * renderable content, and each tile as an associated bounding volume.
     * 
     * The actual hierarchy is represented with the {@link Tile::getParent}
     * and {@link Tile::getChildren} functions.
     * 
     * The renderable content is provided as a {@link TileContentLoadResult}
     * from the {@link Tile::getContent} function. Tiles may have content with 
     * different levels of detail. The {@link Tile::getGeometricError} function 
     * returns the geometric error of the representation of the renderable 
     * content of a tile.
     * 
     * The {@link BoundingVolume} is given by the {@link Tile::getBoundingVolume}
     * function. This bounding volme closes the renderable content of the 
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
            Destroying = -2,

            /**
             * @brief Something went wrong while loading this tile.
             */
            Failed = -1,

            /**
             * @brief The tile is not yet loaded at all, beyond the metadata in tileset.json.
             */
            Unloaded = 0,

            /**
             * @brief The tile content is currently being loaded. 
             *
             * Note that while a tile is in this state, its {@link Tile::getContent}, {@link Tile::setContent},
             * {@link Tile::getState}, and {@link Tile::setState} methods may be called from the load thread.
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
         * @brief Default constructor for an empty, uninitialized tile
         */
        Tile();

        /**
         * @brief Default destructor, which clears all resources associated with this tile
         */
        ~Tile();

        /**
         * @brief Copy constructor
         *
         * @param rhs The other instance
         */
        Tile(Tile& rhs) noexcept = delete;

        /**
         * @brief Copy constructor
         * 
         * @param rhs The other instance
         */
        Tile(Tile&& rhs) noexcept;

        /**
         * @brief Assignment operator
         *
         * @param rhs The other instance
         */
        Tile& operator=(Tile&& rhs) noexcept;

        void prepareToDestroy();

        Tileset* getTileset() { return this->_pContext->pTileset; }
        const Tileset* getTileset() const { return this->_pContext->pTileset; }
        
        TileContext* getContext() { return this->_pContext; }
        const TileContext* getContext() const { return this->_pContext; }
        void setContext(TileContext* pContext) { this->_pContext = pContext; }

        Tile* getParent() { return this->_pParent; }
        const Tile* getParent() const { return this->_pParent; }
        void setParent(Tile* pParent) { this->_pParent = pParent; }

        gsl::span<const Tile> getChildren() const { return gsl::span<const Tile>(this->_children); }
        gsl::span<Tile> getChildren() { return gsl::span<Tile>(this->_children); }
        void createChildTiles(size_t count);
        void createChildTiles(std::vector<Tile>&& children);

        const BoundingVolume& getBoundingVolume() const { return this->_boundingVolume; }
        void setBoundingVolume(const BoundingVolume& value) { this->_boundingVolume = value; }

        const std::optional<BoundingVolume>& getViewerRequestVolume() const { return this->_viewerRequestVolume; }
        void setViewerRequestVolume(const std::optional<BoundingVolume>& value) { this->_viewerRequestVolume = value; }

        double getGeometricError() const { return this->_geometricError; }
        void setGeometricError(double value) { this->_geometricError = value; }

        TileRefine getRefine() const { return this->_refine; }
        void setRefine(TileRefine value) { this->_refine = value; }

        /**
        * @brief Gets the transformation matrix for this tile. 
        *
        * This matrix does _not_ need to be multiplied with the tile's parent's transform 
        * as this has already been done.
        */
        const glm::dmat4x4& getTransform() const { return this->_transform; }
        void setTransform(const glm::dmat4x4& value) { this->_transform = value; }

        const TileID& getTileID() const { return this->_id; }
        void setTileID(const TileID& id);

        const std::optional<BoundingVolume>& getContentBoundingVolume() const { return this->_contentBoundingVolume; }
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

        void* getRendererResources() const { return this->_pRendererResources; }

        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }

        TileSelectionState& getLastSelectionState() { return this->_lastSelectionState; }
        const TileSelectionState& getLastSelectionState() const { return this->_lastSelectionState; }
        void setLastSelectionState(const TileSelectionState& newState) { this->_lastSelectionState = newState; }

        /**
         * @brief Determines if this tile is currently renderable.
         */
        bool isRenderable() const;

        /**
         * @brief Trigger the process of loading the {@link Tile::getContent}
         * 
         * This function is not supposed to be called by clients.
         * 
         * If this tile was already unloaded (indicated by the {@link Tile::getState} of 
         * this tile being {@link Tile::LoadState::Unloaded}), then nothing will be done.
         * 
         * Otherwise, the tile will go into the {@link Tile::LoadState::ContentLoading}
         * state, and the request for loading the tile content will be sent out. The
         * response of the request will be received asynchronously. Depending on the
         * type of the tile and the response, the tile will eventually go into the
         * {@link Tile::LoadState::ContentLoaded} state, and the {@link Tile::getContent}
         * will be available.
         */
        void loadContent();

        /**
         * @brief Frees all resources that have been allocated for the {@link Tile::getContent}.
         * 
         * This function is not supposed to be called by clients.
         *
         * If this tile was already unloaded (indicated by the {@link Tile::getState} of
         * this tile being {@link Tile::LoadState::Unloaded}), then nothing will be done.
         * 
         * Otherwise, the resources that have been allocated for the tile content will
         * be freed.
         * 
         * @return Whether the content was unloaded.
         */
        bool unloadContent();

        /**
         * @brief Gives this tile a chance to update itself each render frame.
         * 
         * @param previousFrameNumber The number of the previous render frame.
         * @param currentFrameNumber The number of the current render frame.
         */
        void update(uint32_t previousFrameNumber, uint32_t currentFrameNumber);

        CesiumUtility::DoublyLinkedListPointers<Tile> _loadedTilesLinks;

    protected:
        void setState(LoadState value);

        /**
         * @brief Will be called when the response for loading the tile content was received.
         * 
         * This will examine the response of the given {@link IAssetRequest} and
         * create the {@link Tile::getContent} of this tile. The requests are 
         * triggered by {@link Tile::loadContent}, and this function will be
         * called asynchronously when the response arrives. 
         * 
         * @param pRequest The {@link IAssetRequest} for which the response was received.
         */
        void contentResponseReceived(IAssetRequest* pRequest);
        void generateTextureCoordinates();
        void upsampleParent();

    private:
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
        std::unique_ptr<IAssetRequest> _pContentRequest;
        std::unique_ptr<TileContentLoadResult> _pContent;
        void* _pRendererResources;

        // Selection state
        TileSelectionState _lastSelectionState;

        // Overlays
        std::vector<RasterMappedTo3DTile> _rasterTiles;
    };

}
