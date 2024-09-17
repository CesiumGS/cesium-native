#pragma once

#include "CesiumGltf/ImageCesium.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <cstddef>
#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace CesiumGltf {

namespace SharedAssetDepotInternals {

template <typename AssetType> class SharedAsset;
template <typename AssetType> class SingleAssetDepot;

/**
 * Contains the current state of an asset within the SharedAssetDepot.
 */
template <typename AssetType>
class AssetContainer
    : public CesiumUtility::ReferenceCounted<AssetContainer<AssetType>, true> {
public:
  std::atomic<std::int32_t> counter;
  std::string assetId;
  AssetType asset;

  // Pointer to this container's parent so we know how to clean it up.
  SingleAssetDepot<AssetType>* parent;

  AssetContainer(
      std::string assetId_,
      AssetType&& asset_,
      SingleAssetDepot<AssetType>* parent_)
      : counter(0),
        assetId(assetId_),
        asset(std::move(asset_)),
        parent(parent_) {}
  SharedAsset<AssetType> toRef() { return SharedAsset(this); }
};

/**
 * An asset that may or may not be stored in the SharedAssetDepot and shared
 * across multiple tiles. This can either be an ImageCesium itself or a
 * reference-counted pointer to an entry in the SharedAssetDepot. You should
 * always treat it as if it was an ImageCesium, not a smart pointer.
 * @tparam AssetType The type of the asset that we're getting a reference to.
 */
template <typename AssetType> class SharedAsset {
public:
  SharedAsset(AssetType&& asset)
      : contents(new AssetContainer<AssetType>(
            std::string(),
            std::forward<AssetType>(asset),
            nullptr)) {}
  SharedAsset(AssetType& asset)
      : contents(new AssetContainer<AssetType>(
            std::string(),
            std::forward<AssetType>(asset),
            nullptr)) {}
  SharedAsset(std::nullptr_t) : contents(nullptr) {}
  SharedAsset()
      : contents(new AssetContainer<AssetType>(
            std::string(),
            AssetType(),
            nullptr)) {}

  AssetType* get() { return &this->contents->asset; }

  const AssetType* get() const { return &this->contents->asset; }

  AssetType* operator->() { return this->get(); }
  const AssetType* operator->() const { return this->get(); }
  AssetType& operator*() { return *this->get(); }
  const AssetType& operator*() const { return *this->get(); }

  bool operator==(const SharedAsset<AssetType>& other) const {
    return other.get() == this->get();
  }

  bool operator==(SharedAsset<AssetType>& other) {
    return other.get() == this->get();
  }

  bool operator!=(const SharedAsset<AssetType>& other) const {
    return other.get() != this->get();
  }

  bool operator!=(SharedAsset<AssetType>& other) {
    return other.get() != this->get();
  }

  /**
   * Copy assignment operator for SharedAsset.
   */
  void operator=(const SharedAsset<AssetType>& other) {
    if (*this != other) {
      // Decrement this reference
      this->changeCounter(-1);
      this->contents = other.contents;
      // Increment the new reference
      this->changeCounter(1);
    }
  }

  /**
   * Move assignment operator for SharedAsset.
   */
  void operator=(SharedAsset<AssetType>&& other) noexcept {
    if (*this != other) {
      this->changeCounter(-1);
      this->contents = std::move(other.contents);
      other.contents = nullptr;
    }
  }

  ~SharedAsset() { this->changeCounter(-1); }

  /**
   * Copy constructor.
   */
  SharedAsset(const SharedAsset<AssetType>& other) {
    contents = other.contents;
    this->changeCounter(1);
  }

  /**
   * Move constructor.
   */
  SharedAsset(SharedAsset<AssetType>&& other) noexcept {
    contents = std::move(other.contents);
    other.contents = nullptr;
  }

private:
  SharedAsset(
      CesiumUtility::IntrusivePointer<AssetContainer<AssetType>> container)
      : contents(container) {
    this->changeCounter(1);
  }

  void changeCounter(int amt) {
    if (contents != nullptr) {
      contents->counter += amt;
      if (contents->counter <= 0) {
        if (contents->parent != nullptr) {
          contents->parent->remove(contents->assetId);
        }
        contents = nullptr;
      }
    }
  }

  // A SharedAssetRef might point to an asset the SharedAssetDepot, or it
  // might not. If it doesn't, we own this asset now.
  CesiumUtility::IntrusivePointer<AssetContainer<AssetType>> contents;

  friend class SharedAssetDepot;
  friend class AssetContainer<AssetType>;
  friend class SingleAssetDepot<AssetType>;
};

/**
 * Contains one or more assets of the given type.
 * @tparam AssetType The type of asset stored in this depot.
 */
template <typename AssetType> class SingleAssetDepot {
public:
  /**
   * Stores the AssetType in this depot and returns a reference to it,
   * or returns the existing asset if present.
   */
  SharedAsset<AssetType> store(std::string& assetId, AssetType&& asset) {
    auto [newIt, added] = this->assets.try_emplace(
        assetId,
        new AssetContainer(assetId, std::move(asset), this));
    return newIt->second.toRef();
  }

  /**
   * Fetches an asset that has a {@link AssetFactory} defined and constructs it if possible.
   * If the asset is already in this depot, it will be returned instead.
   * If the asset has already started loading in this depot but hasn't finished,
   * its future will be returned.
   */
  template <typename Factory>
  CesiumAsync::SharedFuture<std::optional<SharedAsset<AssetType>>> getOrFetch(
      CesiumAsync::AsyncSystem& asyncSystem,
      std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const Factory& factory,
      std::string& uri,
      std::vector<CesiumAsync::IAssetAccessor::THeader> headers) {
    // We need to avoid:
    // - Two assets starting loading before the first asset has updated the
    //   pendingAssets map
    // - An asset starting to load after the previous load has been removed from
    //   the pendingAssets map, but before the completed asset has been added to
    //   the assets map.
    std::lock_guard lock(pendingAssetsMutex);

    auto existingIt = this->assets.find(uri);
    if (existingIt != this->assets.end()) {
      // We've already loaded an asset with this ID - we can just use that.
      return asyncSystem
          .createResolvedFuture(std::optional<SharedAsset<AssetType>>(
              SharedAsset<AssetType>(existingIt->second)))
          .share();
    }

    auto pendingIt = this->pendingAssets.find(uri);
    if (pendingIt != this->pendingAssets.end()) {
      // Return the existing future - the caller can chain off of it.
      return pendingIt->second;
    }

    // We haven't loaded or started to load this asset yet.
    // Let's do that now.
    CesiumAsync::Future<std::optional<SharedAsset<AssetType>>> future =
        pAssetAccessor->get(asyncSystem, uri, headers)
            .thenInWorkerThread(
                [factory = std::move(factory)](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest)
                    -> std::optional<AssetType> {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  if (!pResponse) {
                    return std::nullopt;
                  }

                  return factory.createFrom(pResponse->data());
                })
            // Do this in main thread since we're messing with the collections.
            .thenInMainThread(
                [uri,
                 this,
                 pAssets = &this->assets,
                 pPendingAssetsMutex = &this->pendingAssetsMutex,
                 pPendingAssets =
                     &this->pendingAssets](std::optional<AssetType>&& result)
                    -> std::optional<SharedAsset<AssetType>> {
                  std::lock_guard lock(*pPendingAssetsMutex);

                  // Get rid of our future.
                  pPendingAssets->erase(uri);

                  if (result.has_value()) {
                    auto [it, ok] = pAssets->emplace(
                        uri,
                        new AssetContainer<AssetType>(
                            uri,
                            std::move(result.value()),
                            this));
                    if (!ok) {
                      return std::nullopt;
                    }

                    return std::optional(
                        std::move(SharedAsset<AssetType>(it->second)));
                  }

                  return std::nullopt;
                });

    auto [it, ok] = this->pendingAssets.emplace(uri, std::move(future).share());
    if (!ok) {
      return asyncSystem
          .createResolvedFuture(std::optional<SharedAsset<AssetType>>())
          .share();
    }

    return it->second;
  }

  size_t getDistinctCount() const { return this->assets.size(); }

private:
  /**
   * Removes the given asset from the depot.
   * Should only be called by {@link SharedAsset}.
   */
  void remove(std::string& hash) { this->assets.erase(hash); }

  // Assets that have a unique ID that can be used to de-duplicate them.
  std::unordered_map<
      std::string,
      CesiumUtility::IntrusivePointer<AssetContainer<AssetType>>>
      assets;
  // Futures for assets that still aren't loaded yet.
  std::unordered_map<
      std::string,
      CesiumAsync::SharedFuture<std::optional<SharedAsset<AssetType>>>>
      pendingAssets;
  // Mutex for checking or editing the pendingAssets map
  std::mutex pendingAssetsMutex;

  friend class SharedAsset<AssetType>;
};

/**
 * @brief Contains assets that are potentially shared across multiple tiles.
 */
class SharedAssetDepot {
public:
  SharedAssetDepot() {}

  /**
   * Obtains an existing {@link ImageCesium} or constructs a new one using the provided factory.
   */
  template <typename Factory>
  CesiumAsync::SharedFuture<std::optional<SharedAsset<ImageCesium>>> getOrFetch(
      CesiumAsync::AsyncSystem& asyncSystem,
      std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const Factory& factory,
      std::string& uri,
      std::vector<CesiumAsync::IAssetAccessor::THeader> headers) {
    return images
        .getOrFetch(asyncSystem, pAssetAccessor, factory, uri, headers);
  }

  size_t getImagesCount() const { return this->images.getDistinctCount(); }

private:
  SingleAssetDepot<CesiumGltf::ImageCesium> images;
};

} // namespace SharedAssetDepotInternals

// actually export the public types to the right namespace
// fairly sure this is anti-pattern actually but i'll fix it later
using SharedAssetDepotInternals::SharedAsset;
using SharedAssetDepotInternals::SharedAssetDepot;

} // namespace CesiumGltf
