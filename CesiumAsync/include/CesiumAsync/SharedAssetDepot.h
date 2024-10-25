#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/DoublyLinkedList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <nonstd/expected.hpp>

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace CesiumAsync {

template <typename T> class SharedAsset;

template <typename AssetType> class AssetFactory {
public:
  virtual CesiumUtility::IntrusivePointer<AssetType>
  createFrom(const gsl::span<const gsl::byte>& data) const = 0;
};

struct NetworkAssetKey {
  /**
   * @brief The URL from which this network asset is downloaded.
   */
  std::string url;

  /**
   * @brief The HTTP headers used in requesting this asset.
   */
  std::vector<IAssetAccessor::THeader> headers;

  bool operator==(const NetworkAssetKey& rhs) const noexcept;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the request once it is complete.
   */
  Future<std::shared_ptr<CesiumAsync::IAssetRequest>> loadFromNetwork(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor and return the downloaded bytes.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the downloaded bytes if the request is
   * successful, or a string describing the error if one occurred.
   */
  Future<nonstd::expected<std::vector<std::byte>, std::string>>
  loadBytesFromNetwork(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const;
};

template <typename AssetType> class IDepotOwningAsset {
public:
  virtual ~IDepotOwningAsset() {}
  virtual void markDeletionCandidate(const AssetType& asset) = 0;
  virtual void unmarkDeletionCandidate(const AssetType& asset) = 0;
};

/**
 * A depot for {@link SharedAsset} instances, which are potentially shared between multiple objects.
 * @tparam AssetType The type of asset stored in this depot. This should usually
 * be derived from {@link SharedAsset}.
 */
template <typename AssetType, typename AssetKey = NetworkAssetKey>
class CESIUMASYNC_API SharedAssetDepot
    : public CesiumUtility::ReferenceCountedThreadSafe<
          SharedAssetDepot<AssetType>>,
      public IDepotOwningAsset<AssetType> {
public:
  /**
   * @brief The maximum total byte usage of assets that have been loaded but are
   * no longer needed.
   *
   * When cached assets are no longer needed, they're marked as
   * candidates for deletion. However, this deletion doesn't actually occur
   * until the total byte usage of deletion candidates exceeds this threshold.
   * At that point, assets are cleaned up in the order that they were marked for
   * deletion until the total dips below this threshold again.
   *
   * Default is 16MiB.
   */
  int64_t staleAssetSizeLimit = 16 * 1024 * 1024;

  using FactorySignature =
      CesiumAsync::Future<CesiumUtility::IntrusivePointer<AssetType>>(
          const AsyncSystem& asyncSystem,
          const AssetKey& key);

  SharedAssetDepot(std::function<FactorySignature> factory)
      : _factory(std::move(factory)) {}

  ~SharedAssetDepot() {
    // It's possible the assets will outlive the depot, if they're still in use.
    std::lock_guard lock(this->_assetsMutex);

    for (auto& pair : this->_assets) {
      // Transfer ownership to the reference counting system.
      CesiumUtility::IntrusivePointer<AssetType> pCounted =
          pair.second.release();
      pCounted->_pDepot = nullptr;
    }
  }

  /**
   * Stores the AssetType in this depot and returns a reference to it,
   * or returns the existing asset if present.
   */
  CesiumUtility::IntrusivePointer<AssetType> store(
      const AssetKey& assetId,
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
    std::lock_guard lock(this->_assetsMutex);

    auto findIt = this->_assets.find(assetId);
    if (findIt != this->_assets.end()) {
      // This asset ID already exists in the depot, so we can't add this asset.
      return findIt->second.get();
    }

    // If this asset ID is pending, but hasn't completed yet, where did this
    // asset come from? It shouldn't happen.
    CESIUM_ASSERT(
        this->_pendingAssets.find(assetId) == this->_pendingAssets.end());

    pAsset->_pDepot = this;
    this->_assetKeys[pAsset.get()] = assetId;

    // Now that this asset is owned by the depot, we exclusively
    // control its lifetime with a std::unique_ptr.
    std::unique_ptr<AssetType> pOwnedAsset(pAsset.get());

    auto [addIt, added] =
        this->_assets.emplace(assetId, std::move(pOwnedAsset));

    // We should always add successfully, because we verified it didn't already
    // exist. A failed emplace is disastrous because our unique_ptr will end up
    // destroying the user's object, which may still be in use.
    CESIUM_ASSERT(added);

    return addIt->second.get();
  }

  /**
   * Fetches an asset that has a {@link AssetFactory} defined and constructs it if possible.
   * If the asset is already in this depot, it will be returned instead.
   * If the asset has already started loading in this depot but hasn't finished,
   * its future will be returned.
   */
  CesiumAsync::SharedFuture<CesiumUtility::IntrusivePointer<AssetType>>
  getOrFetch(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const AssetKey& assetKey) {
    // We need to avoid:
    // - Two assets starting to load before the first asset has updated the
    //   pendingAssets map
    // - An asset starting to load after the previous load has been removed from
    //   the pendingAssets map, but before the completed asset has been added to
    //   the assets map.
    std::lock_guard lock(this->_assetsMutex);

    auto existingIt = this->_assets.find(assetKey);
    if (existingIt != this->_assets.end()) {
      // We've already loaded an asset with this ID - we can just use that.
      return asyncSystem
          .createResolvedFuture(CesiumUtility::IntrusivePointer<AssetType>(
              existingIt->second.get()))
          .share();
    }

    auto pendingIt = this->_pendingAssets.find(assetKey);
    if (pendingIt != this->_pendingAssets.end()) {
      // Return the existing future - the caller can chain off of it.
      return pendingIt->second;
    }

    // We haven't loaded or started to load this asset yet.
    // Let's do that now.
    CesiumAsync::Future<CesiumUtility::IntrusivePointer<AssetType>> future =
        this->_factory(asyncSystem, assetKey)
            .thenInWorkerThread(
                [assetKey,
                 this](CesiumUtility::IntrusivePointer<AssetType>&& pResult)
                    -> CesiumUtility::IntrusivePointer<AssetType> {
                  std::lock_guard lock(this->_assetsMutex);

                  // Remove pending asset.
                  this->_pendingAssets.erase(assetKey);

                  // Store the new asset in the depot.
                  return this->store(assetKey, pResult);
                });

    auto [it, added] =
        this->_pendingAssets.emplace(assetKey, std::move(future).share());

    // Should always be added successfully, because we checked above that the
    // asset key doesn't exist in the map yet.
    CESIUM_ASSERT(added);

    return it->second;
  }

  /**
   * @brief Returns the total number of distinct assets contained in this depot.
   */
  size_t getDistinctCount() const { return this->_assets.size(); }

  /**
   * @brief Returns the number of active references to assets in this depot.
   */
  size_t getUsageCount() const {
    size_t count = 0;
    for (const auto& [key, item] : _assets) {
      count += item->_referenceCount;
    }
    return count;
  }

  size_t getDeletionCandidateCount() const {
    std::lock_guard lock(this->_deletionCandidatesMutex);
    return this->_deletionCandidates.size();
  }

  int64_t getDeletionCandidateTotalSizeBytes() const {
    return this->_totalDeletionCandidateMemoryUsage;
  }

private:
  // Disable copy
  void operator=(const SharedAssetDepot<AssetType>& other) = delete;

  /**
   * Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void markDeletionCandidate(const AssetType& asset) {
    std::lock_guard lock(this->_deletionCandidatesMutex);

    asset._sizeInDepot = asset.getSizeBytes();
    this->_totalDeletionCandidateMemoryUsage += asset._sizeInDepot;

    this->_deletionCandidates.insertAtTail(const_cast<AssetType&>(asset));

    if (this->_totalDeletionCandidateMemoryUsage > this->staleAssetSizeLimit) {
      std::lock_guard assetsLock(this->_assetsMutex);

      // Delete the deletion candidates until we're below the limit.
      while (this->_deletionCandidates.size() > 0 &&
             this->_totalDeletionCandidateMemoryUsage >
                 this->staleAssetSizeLimit) {
        AssetType* pOldAsset = this->_deletionCandidates.head();
        this->_deletionCandidates.remove(const_cast<AssetType&>(*pOldAsset));

        this->_totalDeletionCandidateMemoryUsage -= pOldAsset->_sizeInDepot;

        CESIUM_ASSERT(pOldAsset->_referenceCount == 0);

        auto keyIt = this->_assetKeys.find(pOldAsset);
        CESIUM_ASSERT(keyIt != this->_assetKeys.end());
        if (keyIt != this->_assetKeys.end()) {
          // This will actually delete the asset.
          this->_assets.erase(keyIt->second);
        }
      }
    }
  }

  /**
   * Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void unmarkDeletionCandidate(const AssetType& asset) {
    std::lock_guard lock(this->_deletionCandidatesMutex);

    // We should only be deleting this element if it's in the deletionCandidates
    // list.
    bool isFound = !asset._deletionListPointers.isOrphan() ||
                   (this->_deletionCandidates.size() == 1 &&
                    this->_deletionCandidates.head() == &asset);

    CESIUM_ASSERT(isFound);

    if (isFound) {
      this->_totalDeletionCandidateMemoryUsage -= asset._sizeInDepot;
      this->_deletionCandidates.remove(const_cast<AssetType&>(asset));
    }
  }

  // Assets that have a unique ID that can be used to de-duplicate them.
  std::unordered_map<AssetKey, std::unique_ptr<AssetType>> _assets;
  // Futures for assets that still aren't loaded yet.
  std::unordered_map<
      AssetKey,
      CesiumAsync::SharedFuture<CesiumUtility::IntrusivePointer<AssetType>>>
      _pendingAssets;
  // Maps assets back to the key by which they are known in this depot.
  std::unordered_map<AssetType*, AssetKey> _assetKeys;
  // Mutex for checking or editing the pendingAssets map
  std::mutex _assetsMutex;

  // List of assets that are being considered for deletion, in the order that
  // they became unused.
  CesiumUtility::DoublyLinkedListAdvanced<
      AssetType,
      SharedAsset<AssetType>,
      &SharedAsset<AssetType>::_deletionListPointers>
      _deletionCandidates;
  // The total amount of memory used by all assets in the deletionCandidates
  // list.
  std::atomic<int64_t> _totalDeletionCandidateMemoryUsage;
  // Mutex for modifying the deletionCandidates list.
  mutable std::mutex _deletionCandidatesMutex;

  // The factory used to create new AssetType instances.
  std::function<FactorySignature> _factory;

  friend class SharedAsset<AssetType>;
};

} // namespace CesiumAsync

template <> struct std::hash<CesiumAsync::NetworkAssetKey> {
  std::size_t
  operator()(const CesiumAsync::NetworkAssetKey& key) const noexcept;
};
