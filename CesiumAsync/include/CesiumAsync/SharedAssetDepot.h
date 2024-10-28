#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/DoublyLinkedList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Result.h>

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
  Future<CesiumUtility::Result<std::vector<std::byte>>> loadBytesFromNetwork(
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

  using FactorySignature = CesiumAsync::Future<
      CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>>(
      const AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const AssetKey& key);

  SharedAssetDepot(std::function<FactorySignature> factory)
      : _factory(std::move(factory)) {}

  ~SharedAssetDepot() {
    // It's possible the assets will outlive the depot, if they're still in use.
    for (auto& pair : this->_assets) {
      // Transfer ownership to the reference counting system.
      CesiumUtility::IntrusivePointer<AssetType> pCounted =
          pair.second->pAsset.release();
      pCounted->_pDepot = nullptr;
    }
  }

  /**
   * Fetches an asset that has a {@link AssetFactory} defined and constructs it if possible.
   * If the asset is already in this depot, it will be returned instead.
   * If the asset has already started loading in this depot but hasn't finished,
   * its future will be returned.
   */
  SharedFuture<
      CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>>
  getOrCreate(
      const AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const AssetKey& assetKey) {
    // We need to avoid:
    // - Two assets starting to load before the first asset has updated the
    //   pendingAssets map
    // - An asset starting to load after the previous load has been removed from
    //   the pendingAssets map, but before the completed asset has been added to
    //   the assets map.

    // Calling the factory function while holding the mutex unnecessarily
    // limits parallelism. It can even lead to a bug in the scenario where the
    // `thenInWorkerThread` continuation is invoked immediately in the current
    // thread, before `thenInWorkerThread` itself returns. That would result
    // in an attempt to lock the mutex recursively, which is not allowed.

    // So we jump through some hoops here to publish "this thread is working
    // on it", then unlock the mutex, and _then_ actually call the factory
    // function.
    Promise<void> promise = asyncSystem.createPromise<void>();

    std::unique_lock lock(this->_mutex);

    auto existingIt = this->_assets.find(assetKey);
    if (existingIt != this->_assets.end()) {
      // We've already loaded (or are loading) an asset with this ID - we can
      // just use that.
      const AssetEntry& entry = *existingIt->second;
      if (entry.maybePendingAsset) {
        // Asset is currently loading.
        return *entry.maybePendingAsset;
      } else {
        // Asset is already loaded (or failed to load).
        return asyncSystem.createResolvedFuture(entry.toResult()).share();
      }
    }

    // We haven't loaded or started to load this asset yet.
    // Let's do that now.
    CesiumUtility::IntrusivePointer<SharedAssetDepot<AssetType, AssetKey>>
        pDepot = this;
    CesiumUtility::IntrusivePointer<AssetEntry> pEntry =
        new AssetEntry(assetKey);

    auto future =
        promise.getFuture()
            .thenImmediately([pDepot, pEntry, asyncSystem, pAssetAccessor]() {
              return pDepot->_factory(asyncSystem, pAssetAccessor, pEntry->key);
            })
            .thenInWorkerThread(
                [pDepot,
                 pEntry](CesiumUtility::Result<
                         CesiumUtility::IntrusivePointer<AssetType>>&& result) {
                  std::lock_guard lock(pDepot->_mutex);

                  if (result.pValue) {
                    result.pValue->_pDepot = pDepot.get();
                    pDepot->_assetsByPointer[result.pValue.get()] =
                        pEntry.get();
                  }

                  // Now that this asset is owned by the depot, we exclusively
                  // control its lifetime with a std::unique_ptr.
                  pEntry->pAsset =
                      std::unique_ptr<AssetType>(result.pValue.get());
                  pEntry->errorsAndWarnings = std::move(result.errors);
                  pEntry->maybePendingAsset.reset();

                  return pEntry->toResult();
                });

    SharedFuture<
        CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>>
        sharedFuture = std::move(future).share();

    pEntry->maybePendingAsset = sharedFuture;

    auto [it, added] = this->_assets.emplace(assetKey, pEntry);

    // Should always be added successfully, because we checked above that the
    // asset key doesn't exist in the map yet.
    CESIUM_ASSERT(added);

    // Unlock the mutex and then call the factory function.
    lock.unlock();
    promise.resolve();

    return sharedFuture;
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
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   */
  void markDeletionCandidate(const AssetType& asset) override {
    std::lock_guard lock(this->_mutex);

    auto it = this->_assetsByPointer.find(const_cast<AssetType*>(&asset));
    CESIUM_ASSERT(it != this->_assetsByPointer.end());
    if (it == this->_assetsByPointer.end()) {
      return;
    }

    CESIUM_ASSERT(it->second != nullptr);

    AssetEntry& entry = *it->second;
    entry.sizeInDeletionList = asset.getSizeBytes();
    this->_totalDeletionCandidateMemoryUsage += entry.sizeInDeletionList;

    this->_deletionCandidates.insertAtTail(entry);

    if (this->_totalDeletionCandidateMemoryUsage > this->staleAssetSizeLimit) {
      // Delete the deletion candidates until we're below the limit.
      while (this->_deletionCandidates.size() > 0 &&
             this->_totalDeletionCandidateMemoryUsage >
                 this->staleAssetSizeLimit) {
        AssetEntry* pOldEntry = this->_deletionCandidates.head();
        this->_deletionCandidates.remove(*pOldEntry);

        this->_totalDeletionCandidateMemoryUsage -=
            pOldEntry->sizeInDeletionList;

        CESIUM_ASSERT(
            pOldEntry->pAsset == nullptr ||
            pOldEntry->pAsset->_referenceCount == 0);

        if (pOldEntry->pAsset) {
          this->_assetsByPointer.erase(pOldEntry->pAsset.get());
        }

        // This will actually delete the asset.
        this->_assets.erase(pOldEntry->key);
      }
    }
  }

  /**
   * Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   */
  void unmarkDeletionCandidate(const AssetType& asset) override {
    std::lock_guard lock(this->_mutex);

    auto it = this->_assetsByPointer.find(const_cast<AssetType*>(&asset));
    CESIUM_ASSERT(it != this->_assetsByPointer.end());
    if (it == this->_assetsByPointer.end()) {
      return;
    }

    CESIUM_ASSERT(it->second != nullptr);

    AssetEntry& entry = *it->second;
    bool isFound = this->_deletionCandidates.contains(entry);

    CESIUM_ASSERT(isFound);

    if (isFound) {
      this->_totalDeletionCandidateMemoryUsage -= entry.sizeInDeletionList;
      this->_deletionCandidates.remove(entry);
    }
  }

  /**
   * @brief An entry for an asset owned by this depot. This is reference counted
   * so that we can keep it alive during async operations.
   */
  struct AssetEntry
      : public CesiumUtility::ReferenceCountedThreadSafe<AssetEntry> {
    AssetEntry(AssetKey&& key_)
        : CesiumUtility::ReferenceCountedThreadSafe<AssetEntry>(),
          key(std::move(key_)),
          pAsset(),
          maybePendingAsset(),
          errorsAndWarnings(),
          sizeInDeletionList(0),
          deletionListPointers() {}

    AssetEntry(const AssetKey& key_) : AssetEntry(AssetKey(key_)) {}

    /**
     * @brief The unique key identifying this asset.
     */
    AssetKey key;

    /**
     * @brief A pointer to the asset. This may be nullptr if the asset is still
     * being loaded, or if it failed to load.
     */
    std::unique_ptr<AssetType> pAsset;

    /**
     * @brief If this asset is currently loading, this field holds a shared
     * future that will resolve when the asset load is complete. This field will
     * be empty if the asset finished loading, including if it failed to load.
     */
    std::optional<SharedFuture<
        CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>>>
        maybePendingAsset;

    /**
     * @brief The errors and warnings that occurred while loading this asset.
     * This will not contain any errors or warnings if the asset has not
     * finished loading yet.
     */
    CesiumUtility::ErrorList errorsAndWarnings;

    /**
     * @brief The size of this asset when it was added to the
     * _deletionCandidates list. This is stored so that the exact same size can
     * be subtracted later. The value of this field is undefined if the asset is
     * not currently in the _deletionCandidates list.
     */
    int64_t sizeInDeletionList;

    /**
     * @brief The next and previous pointers to entries in the
     * _deletionCandidates list.
     */
    CesiumUtility::DoublyLinkedListPointers<AssetEntry> deletionListPointers;

    CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>
    toResult() const {
      return CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>(
          pAsset.get(),
          errorsAndWarnings);
    }

    SharedFuture<
        CesiumUtility::Result<CesiumUtility::IntrusivePointer<AssetType>>>
    toFuture(const AsyncSystem& asyncSystem) const {
      if (this->maybePendingAsset) {
        return *this->maybePendingAsset;
      } else {
        return asyncSystem.createResolvedFuture(this->toResult()).share();
      }
    }
  };

  // Maps asset keys to AssetEntry instances. This collection owns the asset
  // entries.
  std::unordered_map<AssetKey, CesiumUtility::IntrusivePointer<AssetEntry>>
      _assets;

  // Maps asset pointers to AssetEntry instances. The values in this map refer
  // to instances owned by the _assets map.
  std::unordered_map<AssetType*, AssetEntry*> _assetsByPointer;

  // List of assets that are being considered for deletion, in the order that
  // they became unused.
  CesiumUtility::DoublyLinkedList<AssetEntry, &AssetEntry::deletionListPointers>
      _deletionCandidates;

  // The total amount of memory used by all assets in the _deletionCandidates
  // list.
  std::atomic<int64_t> _totalDeletionCandidateMemoryUsage;

  // Mutex serializing access to _assets, _assetsByPointer, _deletionCandidates,
  // and any AssetEntry owned by this depot.
  std::mutex _mutex;

  // The factory used to create new AssetType instances.
  std::function<FactorySignature> _factory;

  friend class SharedAsset<AssetType>;
};

} // namespace CesiumAsync

template <> struct std::hash<CesiumAsync::NetworkAssetKey> {
  std::size_t
  operator()(const CesiumAsync::NetworkAssetKey& key) const noexcept;
};
