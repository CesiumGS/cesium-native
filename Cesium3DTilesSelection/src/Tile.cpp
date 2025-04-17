#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Model.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Math.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef CESIUM_DEBUG_TILE_UNLOADING
#include <cpptrace/cpptrace.hpp>

#include <unordered_map>
#endif

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace std::string_literals;

namespace Cesium3DTilesSelection {
#ifdef CESIUM_DEBUG_TILE_UNLOADING
std::unordered_map<
    std::string,
    std::vector<TileDoNotUnloadSubtreeCountTracker::Entry>>
    TileDoNotUnloadSubtreeCountTracker::_entries;

void TileDoNotUnloadSubtreeCountTracker::addEntry(
    uint64_t id,
    bool increment,
    const char* reason,
    int32_t newCount) {
  const std::string idString = fmt::format("{:x}", id);
  const auto foundIt =
      TileDoNotUnloadSubtreeCountTracker::_entries.find(idString);
  if (foundIt != TileDoNotUnloadSubtreeCountTracker::_entries.end()) {
    foundIt->second.push_back(Entry{reason, increment, newCount});
  } else {
    std::vector<Entry> entries{Entry{reason, increment, newCount}};

    TileDoNotUnloadSubtreeCountTracker::_entries.insert(
        {idString, std::move(entries)});
  }
}
#endif

Tile::Tile(TilesetContentLoader* pLoader) noexcept
    : Tile(TileConstructorImpl{}, TileLoadState::Unloaded, pLoader) {}

Tile::Tile(
    TilesetContentLoader* pLoader,
    std::unique_ptr<TileExternalContent>&& externalContent) noexcept
    : Tile(
          TileConstructorImpl{},
          TileLoadState::ContentLoaded,
          pLoader,
          TileContent(std::move(externalContent))) {}

Tile::Tile(
    TilesetContentLoader* pLoader,
    TileEmptyContent emptyContent) noexcept
    : Tile(
          TileConstructorImpl{},
          TileLoadState::ContentLoaded,
          pLoader,
          emptyContent) {}

template <typename... TileContentArgs, typename TileContentEnable>
Tile::Tile(
    TileConstructorImpl,
    TileLoadState loadState,
    TilesetContentLoader* pLoader,
    TileContentArgs&&... args)
    : _pParent(nullptr),
      _children(),
      _id(""s),
      _boundingVolume(OrientedBoundingBox(glm::dvec3(), glm::dmat3())),
      _viewerRequestVolume(),
      _contentBoundingVolume(),
      _geometricError(0.0),
      _refine(TileRefine::Replace),
      _transform(1.0),
      _unusedTilesLinks(),
      _content{std::forward<TileContentArgs>(args)...},
      _pLoader{pLoader},
      _loadState{loadState},
      _mightHaveLatentChildren{true},
      _rasterTiles(),
      _referenceCount(0) {
  if (!this->_content.isUnknownContent()) {
    // Add a reference for the loaded content.
    this->addReference("Constructor with content");
  }
}

Tile::Tile(Tile&& rhs) noexcept
    : _pParent(nullptr),
      _children(std::move(rhs._children)),
      _id(std::move(rhs._id)),
      _boundingVolume(rhs._boundingVolume),
      _viewerRequestVolume(rhs._viewerRequestVolume),
      _contentBoundingVolume(rhs._contentBoundingVolume),
      _geometricError(rhs._geometricError),
      _refine(rhs._refine),
      _transform(rhs._transform),
      _unusedTilesLinks(),
      _content(std::move(rhs._content)),
      _pLoader{rhs._pLoader},
      _loadState{rhs._loadState},
      _mightHaveLatentChildren{rhs._mightHaveLatentChildren},
      _rasterTiles(std::move(rhs._rasterTiles)),
      // See the move assignment operator for an explanation of how we copy
      // `_referenceCount` here.
      _referenceCount(0) {
  if (!this->_content.isUnknownContent()) {
    this->addReference("Move constructor with content");
    rhs.releaseReference("RHS passed to move constructor");
  }

  // since children of rhs will have the parent pointed to rhs,
  // we will reparent them to this tile as rhs will be destroyed after this
  for (Tile& tile : this->_children) {
    tile.setParent(this);
  }

  this->setParent(rhs._pParent);
  rhs.setParent(nullptr);
}

Tile::~Tile() noexcept {
  // A tile being destroyed should not have any references. Except that the root
  // tile's content will never be unloaded, so its reference count may be 1.
  this->clearChildren();
  CESIUM_ASSERT(
      this->_referenceCount == 0 ||
      (this->_pParent == nullptr && this->_referenceCount == 1));
}

void Tile::createChildTiles(std::vector<Tile>&& children) {
  if (!this->_children.empty()) {
    throw std::runtime_error("Children already created.");
  }

  this->_children = std::move(children);
  for (Tile& tile : this->_children) {
    CESIUM_ASSERT(tile.getParent() == nullptr);
    tile.setParent(this);
  }
}

double Tile::getNonZeroGeometricError() const noexcept {
  double geometricError = this->getGeometricError();
  if (geometricError > Math::Epsilon5) {
    // Tile's geometric error is good.
    return geometricError;
  }

  const Tile* pParent = this->getParent();
  double divisor = 1.0;

  while (pParent) {
    if (!pParent->getUnconditionallyRefine()) {
      divisor *= 2.0;
      double ancestorError = pParent->getGeometricError();
      if (ancestorError > Math::Epsilon5) {
        return ancestorError / divisor;
      }
    }

    pParent = pParent->getParent();
  }

  // No sensible geometric error all the way to the root of the tile tree.
  // So just use a tiny geometric error and raster selection will be limited by
  // quadtree tile count or texture resolution size.
  return Math::Epsilon5;
}

int64_t Tile::computeByteSize() const noexcept {
  int64_t bytes = 0;

  const TileContent& content = this->getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent) {
    const CesiumGltf::Model& model = pRenderContent->getModel();

    // Add up the glTF buffers
    for (const CesiumGltf::Buffer& buffer : model.buffers) {
      bytes += int64_t(buffer.cesium.data.size());
    }

    const std::vector<CesiumGltf::BufferView>& bufferViews = model.bufferViews;
    for (const CesiumGltf::Image& image : model.images) {
      const int32_t bufferView = image.bufferView;
      // For images loaded from buffers, subtract the buffer size before adding
      // the decoded image size.
      if (bufferView >= 0 &&
          bufferView < static_cast<int32_t>(bufferViews.size())) {
        bytes -= bufferViews[size_t(bufferView)].byteLength;
      }

      // sizeBytes is set in TilesetContentManager::ContentKindSetter, if not
      // sooner (e.g., by the renderer implementation).
      if (image.pAsset) {
        bytes += image.pAsset->sizeBytes;
      }
    }
  }

  return bytes;
}

bool Tile::isRenderable() const noexcept {
  if (getState() == TileLoadState::Failed) {
    // Explicitly treat failed tiles as "renderable" - we just treat them like
    // empty tiles.
    return true;
  }

  if (getState() == TileLoadState::Done) {
    // An unconditionally-refined tile is never renderable... UNLESS it has no
    // children, in which case waiting longer will be futile.
    if (!getUnconditionallyRefine() || this->_children.empty()) {
      return std::all_of(
          this->_rasterTiles.begin(),
          this->_rasterTiles.end(),
          [](const RasterMappedTo3DTile& rasterTile) noexcept {
            return rasterTile.getReadyTile() != nullptr;
          });
    }
  }

  return false;
}

bool Tile::isRenderContent() const noexcept {
  return this->_content.isRenderContent();
}

bool Tile::isExternalContent() const noexcept {
  return this->_content.isExternalContent();
}

bool Tile::isEmptyContent() const noexcept {
  return this->_content.isEmptyContent();
}

TilesetContentLoader* Tile::getLoader() const noexcept {
  return this->_pLoader;
}

TileLoadState Tile::getState() const noexcept { return this->_loadState; }

namespace {

bool anyRasterOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    const CesiumRasterOverlays::RasterOverlayTile* pLoading =
        mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() ==
            CesiumRasterOverlays::RasterOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

} // namespace

bool Tile::needsWorkerThreadLoading() const noexcept {
  TileLoadState state = this->getState();
  return state == TileLoadState::Unloaded ||
         state == TileLoadState::FailedTemporarily ||
         anyRasterOverlaysNeedLoading(*this);
}

bool Tile::needsMainThreadLoading() const noexcept {
  return this->getState() == TileLoadState::ContentLoaded &&
         this->isRenderContent();
}

void Tile::setParent(Tile* pParent) noexcept {
  if (this->getReferenceCount() > 0) {
    // Release the reference to the previous parent, or to the
    // TilesetContentManager if this Tile previously didn't have a parent.
    if (this->_pParent != nullptr) {
      this->_pParent->releaseReference("Release reference to old parent");
    } else if (this->_pLoader && this->_pLoader->getOwner() != nullptr) {
      this->_pLoader->getOwner()->releaseReference();
    }
  }

  this->_pParent = pParent;

  if (this->getReferenceCount() > 0) {
    // Add a reference to the new parent, or to the
    // TilesetContentManager if this Tile doesn't have a parent.
    if (this->_pParent != nullptr) {
      this->_pParent->addReference("Add reference to new parent");
    } else if (this->_pLoader && this->_pLoader->getOwner() != nullptr) {
      this->_pLoader->getOwner()->addReference();
    }
  }
}

void Tile::setState(TileLoadState state) noexcept { this->_loadState = state; }

bool Tile::getMightHaveLatentChildren() const noexcept {
  return this->_mightHaveLatentChildren;
}

void Tile::setMightHaveLatentChildren(bool mightHaveLatentChildren) noexcept {
  this->_mightHaveLatentChildren = mightHaveLatentChildren;
}

void Tile::clearChildren() noexcept {
  CESIUM_ASSERT(!std::any_of(
      this->_children.begin(),
      this->_children.end(),
      [](const Tile& child) { return child.getReferenceCount() > 0; }));
  this->_children.clear();
}

namespace {

// Is this tile's content referenced? We can tell by careful inspection of the
// reference count.
bool isContentReferenced(const Tile& tile) {
  int32_t referencesNotToThisTilesContent = 0;

  // A reference to the current tile as a result of its content being loaded.
  if (!tile.getContent().isUnknownContent())
    ++referencesNotToThisTilesContent;

  // References from a (referenced) child to the parent.
  for (const Tile& child : tile.getChildren()) {
    if (child.getReferenceCount() > 0)
      ++referencesNotToThisTilesContent;
  }

  // If there are any further references, other than the ones counted above,
  // it means there's some other reference to this Tile, and it should prevent
  // the tile's content from unloading. An assertion failure here means that
  // somehow the child and content references haven't be counted correctly.
  CESIUM_ASSERT(tile.getReferenceCount() >= referencesNotToThisTilesContent);
  return tile.getReferenceCount() > referencesNotToThisTilesContent;
}

} // namespace

void Tile::addReference(const char* reason) const noexcept {
  ++this->_referenceCount;

#ifdef CESIUM_DEBUG_TILE_UNLOADING
  TileDoNotUnloadSubtreeCountTracker::addEntry(
      reinterpret_cast<uint64_t>(this),
      true,
      reason ? reason : "Unknown",
      this->_referenceCount);
#endif

  // When the reference count goes from 0 to 1, prevent this Tile from being
  // destroyed inadvertently by incrementing its parent's reference count as
  // well.
  if (this->_referenceCount == 1) {
    if (this->_pParent) {
      // An assertion failure here indicates this tile is not in its parent's
      // list of children.
      CESIUM_ASSERT(
          !this->_pParent->_children.empty() &&
          this >= &this->_pParent->_children.front() &&
          this <= &this->_pParent->_children.back());
      this->_pParent->addReference("Parent of referenced tile");
    } else if (this->_pLoader && this->_pLoader->getOwner()) {
      // This is the root tile, or perhaps a tile that hasn't been added to
      // the main tree yet. So keep the TilesetContentManager alive.
      this->_pLoader->getOwner()->addReference();
    }
  }

  // If this reference indicates use of the content, mark this tile
  // "ineligible for content unloading" with the TilesetContentManager.
  if (isContentReferenced(*this) && this->_pLoader &&
      this->_pLoader->getOwner()) {
    this->_pLoader->getOwner()->markTileIneligibleForContentUnloading(*this);
  }
}

void Tile::releaseReference(const char* reason) const noexcept {
  CESIUM_ASSERT(this->_referenceCount > 0);
  --this->_referenceCount;
  int32_t referenceCount = this->_referenceCount;

#ifdef CESIUM_DEBUG_TILE_UNLOADING
  TileDoNotUnloadSubtreeCountTracker::addEntry(
      reinterpret_cast<uint64_t>(this),
      false,
      reason ? reason : "Unknown",
      referenceCount);
#endif

  // If there are no more references that indicate use of the content, mark
  // this tile as "eligible for content unloading" with the
  // TilesetContentManager.
  if (!isContentReferenced(*this) && this->_pLoader &&
      this->_pLoader->getOwner()) {
    this->_pLoader->getOwner()->markTileEligibleForContentUnloading(*this);
  }

  // When the reference count goes from 1 to 0, this Tile is once again
  // eligible for destruction, so release the reference on the parent.
  if (this->_pParent) {
    if (referenceCount == 0) {
      // An assertion failure here indicates this tile is not in its parent's
      // list of children.
      CESIUM_ASSERT(
          !this->_pParent->_children.empty() &&
          this >= &this->_pParent->_children.front() &&
          this <= &this->_pParent->_children.back());
      this->_pParent->releaseReference("Parent of unreferenced tile");
    }
  } else {
    // This is the root tile, or perhaps hasn't been added to the main tree yet.
    // When its reference count goes to zero, we need to remove the reference to
    // the TilesetContentManager that was previously added.
    //
    // However, note that root tiles with external content will never be
    // unloaded (see TilesetContentManager::unloadTileContent), so such a tile's
    // reference count will never go below 1.
    if (referenceCount == 0 ||
        (referenceCount == 1 && this->isExternalContent())) {
      this->_pLoader->getOwner()->releaseReference();
    }
  }
}

int32_t Tile::getReferenceCount() const noexcept {
  return this->_referenceCount;
}

} // namespace Cesium3DTilesSelection
