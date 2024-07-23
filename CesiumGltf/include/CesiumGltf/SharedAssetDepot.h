#pragma once

#include "CesiumGltf/ImageCesium.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>

#include <cstddef>
#include <forward_list>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace CesiumGltf {

namespace SharedAssetDepotInternals {

typedef size_t IdHash;
template <typename AssetType> class SharedAsset;
template <typename AssetType> class SingleAssetDepot;

/**
 * Implemented for assets that can be constructed from data fetched from a URL.
 */
template <typename AssetType> class AssetFactory {
public:
  /**
   * Creates a new instance of this asset out of the given data, if possible.
   */
  virtual std::optional<AssetType>
  createFrom(const gsl::span<const gsl::byte>& data) const = 0;
};

/**
 * Contains the current state of an asset within the SharedAssetDepot.
 */
template <typename AssetType> class AssetContainer {
public:
  uint32_t counter;
  IdHash assetId;
  AssetType asset;

  // Pointer to this container's parent so we know how to clean it up.
  SingleAssetDepot<AssetType>* parent;

  AssetContainer(
      IdHash assetId_,
      AssetType& asset_,
      SingleAssetDepot<AssetType>* parent_)
      : counter(0), assetId(assetId_), asset(asset_), parent(parent_) {}
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
  typedef std::variant<AssetContainer<AssetType>*, AssetType> AssetContents;

public:
  SharedAsset(const AssetType& asset) : contents(asset) {}
  SharedAsset(std::nullptr_t) : contents(nullptr) {}
  SharedAsset() : contents(AssetType()) {}

  AssetType* get() {
    struct Operation {
      AssetType* operator()(AssetType& asset) { return &asset; }

      AssetType* operator()(AssetContainer<AssetType>* container) {
        return &container->asset;
      }
    };

    return std::visit(Operation{}, this->contents);
  }

  const AssetType* get() const {
    struct Operation {
      const AssetType* operator()(const AssetType& asset) { return &asset; }

      const AssetType* operator()(const AssetContainer<AssetType>* container) {
        return &container->asset;
      }
    };

    return std::visit(Operation{}, this->contents);
  }

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
      this->maybeChangeCounter(-1);
      this->contents = other.contents;
      // Increment the new reference
      this->maybeChangeCounter(1);
    }
  }

  /**
   * Move assignment operator for SharedAsset.
   */
  void operator=(SharedAsset<AssetType>&& other) noexcept {
    if (*this != other) {
      this->maybeChangeCounter(-1);
      this->contents = std::move(other.contents);
      other.contents = nullptr;
    }
  }

  ~SharedAsset() { this->maybeChangeCounter(-1); }

  /**
   * Copy constructor.
   */
  SharedAsset(const SharedAsset<AssetType>& other) {
    contents = other.contents;
    this->maybeChangeCounter(1);
  }

  /**
   * Move constructor.
   */
  SharedAsset(SharedAsset<AssetType>&& other) noexcept {
    contents = std::move(other.contents);
    other.contents = nullptr;
  }

private:
  SharedAsset(AssetContainer<AssetType>* container) : contents(container) {
    this->maybeChangeCounter(1);
  }

  void maybeChangeCounter(int amt) {
    struct Operation {
      int amt;

      void operator()(AssetContainer<AssetType>* asset) {
        if (asset != nullptr) {
          asset->counter += this->amt;
          if (asset->counter <= 0) {
            asset->parent->remove(asset->assetId);
          }
        }
      }

      void operator()(AssetType& asset) {}
    };

    std::visit(Operation{amt}, this->contents);
  }

  // A SharedAssetRef might point to an asset the SharedAssetDepot, or it
  // might not. If it doesn't, we own this asset now.
  AssetContents contents;

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
  SingleAssetDepot() {}

  /**
   * Stores the AssetType in this depot and returns a reference to it,
   * or returns the existing asset if present.
   */
  SharedAsset<AssetType> store(IdHash assetId, AssetType& asset) {
    auto it = this->assets.find(assetId);
    if (it != this->assets.end()) {
      // already stored
      return it->second.toRef();
    }

    auto [newIt, ok] =
        this->assets.emplace(assetId, AssetContainer(assetId, asset, this));
    CESIUM_ASSERT(ok);

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
      typename std::enable_if<
          std::is_base_of_v<AssetFactory<AssetType>, Factory>,
          const Factory&>::type factory,
      std::string& uri,
      std::vector<CesiumAsync::IAssetAccessor::THeader> headers) {
    IdHash idHash = std::hash<std::string>{}(uri);

    auto existingIt = this->assets.find(idHash);
    if (existingIt != this->assets.end()) {
      // We've already loaded an asset with this ID - we can just use that.
      return asyncSystem
          .createResolvedFuture(std::optional<SharedAsset<AssetType>>(
              SharedAsset<AssetType>(&existingIt->second)))
          .share();
    }

    auto pendingIt = this->pendingAssets.find(idHash);
    if (pendingIt != this->pendingAssets.end()) {
      // Return the existing future - the caller can chain off of it.
      return pendingIt->second;
    }

    // We haven't loaded or started to load this asset yet.
    // Let's do that now.
    CesiumAsync::Future<std::optional<SharedAsset<AssetType>>> future =
        pAssetAccessor->get(asyncSystem, uri, headers)
            .thenInWorkerThread(
                [factory](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  if (!pResponse) {
                    return std::optional<AssetType>(std::nullopt);
                  }

                  return factory.createFrom(pResponse->data());
                })
            // Do this in main thread since we're messing with the collections.
            .thenInMainThread([idHash,
                               pThiz = this,
                               pAssets = &this->assets,
                               pPendingAssets = &this->pendingAssets](
                                  std::optional<AssetType>& result) {
              // Get rid of our future.
              pPendingAssets->erase(idHash);

              if (result.has_value()) {
                auto [it, ok] = pAssets->emplace(
                    idHash,
                    AssetContainer<AssetType>(idHash, result.value(), pThiz));
                if (!ok) {
                  return std::optional<SharedAsset<AssetType>>();
                }

                return std::optional<SharedAsset<AssetType>>(
                    std::move(SharedAsset<AssetType>(&it->second)));
              }

              return std::optional<SharedAsset<AssetType>>();
            });

    auto& [it, ok] =
        this->pendingAssets.emplace(idHash, std::move(future).share());
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
  void remove(IdHash hash) { this->assets.erase(hash); }

  // Assets that have a unique ID that can be used to de-duplicate them.
  std::unordered_map<IdHash, AssetContainer<AssetType>> assets;
  // Futures for assets that still aren't loaded yet.
  std::unordered_map<
      IdHash,
      CesiumAsync::SharedFuture<std::optional<SharedAsset<AssetType>>>>
      pendingAssets;

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
      typename std::enable_if<
          std::is_base_of_v<AssetFactory<ImageCesium>, Factory>,
          const Factory&>::type factory,
      std::string& uri,
      std::vector<CesiumAsync::IAssetAccessor::THeader> headers) {
    return images.getOrFetch<Factory>(
        asyncSystem,
        pAssetAccessor,
        factory,
        uri,
        headers);
  }

  size_t getImagesCount() const { return this->images.getDistinctCount(); }

private:
  SingleAssetDepot<CesiumGltf::ImageCesium> images;
};

} // namespace SharedAssetDepotInternals

// actually export the public types to the right namespace
// fairly sure this is anti-pattern actually but i'll fix it later
using SharedAssetDepotInternals::AssetFactory;
using SharedAssetDepotInternals::SharedAsset;
using SharedAssetDepotInternals::SharedAssetDepot;

} // namespace CesiumGltf
