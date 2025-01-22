#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/DoublyLinkedList.h>
#include <CesiumUtility/IDepotOwningAsset.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Result.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace CesiumUtility {
template <typename T> class SharedAsset;
}

namespace CesiumAsync {

/**
 * @brief A depot for {@link CesiumUtility::SharedAsset} instances, which are potentially shared between multiple objects.
 *
 * @tparam TAssetType The type of asset stored in this depot. This should
 * be derived from {@link CesiumUtility::SharedAsset}.
 */
template <typename TAssetType, typename TAssetKey>
class CESIUMASYNC_API SharedAssetDepot
    : public CesiumUtility::ReferenceCountedThreadSafe<
          SharedAssetDepot<TAssetType, TAssetKey>>,
      public CesiumUtility::IDepotOwningAsset<TAssetType> {
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
  int64_t inactiveAssetSizeLimitBytes = static_cast<int64_t>(16 * 1024 * 1024);

  /**
   * @brief Signature for the callback function that will be called to fetch and
   * create a new instance of `TAssetType` if one with the given key doesn't
   * already exist in the depot.
   *
   * @param asyncSystem The \ref AsyncSystem used by this \ref SharedAssetDepot.
   * @param pAssetAccessor The \ref IAssetAccessor used by this \ref
   * SharedAssetDepot. Use this to fetch the asset.
   * @param key The `TAssetKey` for the asset that should be loaded by this
   * factory.
   * @returns A \ref CesiumAsync::Future "Future" that resolves to a \ref
   * CesiumUtility::ResultPointer "ResultPointer" containing the loaded asset,
   * or any error information if the asset failed to load.
   */
  using FactorySignature =
      CesiumAsync::Future<CesiumUtility::ResultPointer<TAssetType>>(
          const AsyncSystem& asyncSystem,
          const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
          const TAssetKey& key);

  /**
   * @brief Creates a new `SharedAssetDepot` using the given factory callback to
   * load new assets.
   *
   * @param factory The factory to use to fetch and create assets that don't
   * already exist in the depot. See \ref FactorySignature.
   */
  SharedAssetDepot(std::function<FactorySignature> factory);

  virtual ~SharedAssetDepot();

  /**
   * @brief Gets an asset from the depot if it already exists, or creates it
   * using the depot's factory if it does not.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor to use to download assets, if
   * necessary.
   * @param assetKey The key uniquely identifying the asset to get or create.
   * @return A shared future that resolves when the asset is ready or fails.
   */
  SharedFuture<CesiumUtility::ResultPointer<TAssetType>> getOrCreate(
      const AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const TAssetKey& assetKey);

  /**
   * @brief Returns the total number of distinct assets contained in this depot,
   * including both active and inactive assets.
   */
  size_t getAssetCount() const;

  /**
   * @brief Gets the number of assets owned by this depot that are active,
   * meaning that they are currently being used in one or more places.
   */
  size_t getActiveAssetCount() const;

  /**
   * @brief Gets the number of assets owned by this depot that are inactive,
   * meaning that they are not currently being used.
   */
  size_t getInactiveAssetCount() const;

  /**
   * @brief Gets the total bytes used by inactive (unused) assets owned by this
   * depot.
   */
  int64_t getInactiveAssetTotalSizeBytes() const;

  // Disable copy
  void operator=(const SharedAssetDepot<TAssetType, TAssetKey>& other) = delete;

private:
  struct LockHolder;

  /**
   * @brief Locks the shared asset depot for thread-safe access. It will remain
   * locked until the returned object is destroyed or the `unlock` method is
   * called on it.
   */
  LockHolder lock() const;

  /**
   * @brief Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   *
   * @param asset The asset to mark for deletion.
   * @param threadOwnsDepotLock True if the calling thread already owns the
   * depot lock; otherwise, false.
   */
  void markDeletionCandidate(const TAssetType& asset, bool threadOwnsDepotLock)
      override;

  void markDeletionCandidateUnderLock(const TAssetType& asset);

  /**
   * @brief Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   *
   * @param asset The asset to unmark for deletion.
   * @param threadOwnsDepotLock True if the calling thread already owns the
   * depot lock; otherwise, false.
   */
  void unmarkDeletionCandidate(
      const TAssetType& asset,
      bool threadOwnsDepotLock) override;

  void unmarkDeletionCandidateUnderLock(const TAssetType& asset);

  /**
   * @brief An entry for an asset owned by this depot. This is reference counted
   * so that we can keep it alive during async operations.
   */
  struct AssetEntry
      : public CesiumUtility::ReferenceCountedThreadSafe<AssetEntry> {
    AssetEntry(TAssetKey&& key_)
        : CesiumUtility::ReferenceCountedThreadSafe<AssetEntry>(),
          key(std::move(key_)),
          pAsset(),
          maybePendingAsset(),
          errorsAndWarnings(),
          sizeInDeletionList(0),
          deletionListPointers() {}

    AssetEntry(const TAssetKey& key_) : AssetEntry(TAssetKey(key_)) {}

    /**
     * @brief The unique key identifying this asset.
     */
    TAssetKey key;

    /**
     * @brief A pointer to the asset. This may be nullptr if the asset is still
     * being loaded, or if it failed to load.
     */
    std::unique_ptr<TAssetType> pAsset;

    /**
     * @brief If this asset is currently loading, this field holds a shared
     * future that will resolve when the asset load is complete. This field will
     * be empty if the asset finished loading, including if it failed to load.
     */
    std::optional<SharedFuture<CesiumUtility::ResultPointer<TAssetType>>>
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

    CesiumUtility::ResultPointer<TAssetType> toResultUnderLock() const;
  };

  // Manages the depot's mutex. Also ensures, via IntrusivePointer, that the
  // depot won't be destroyed while the lock is held.
  struct LockHolder {
    LockHolder(
        const CesiumUtility::IntrusivePointer<const SharedAssetDepot>& pDepot);
    ~LockHolder();
    void unlock();

  private:
    // These two fields _must_ be declared in this order to guarantee that the
    // mutex is released before the depot pointer. Releasing the depot pointer
    // could destroy the depot, and that will be disastrous if the lock is still
    // held.
    CesiumUtility::IntrusivePointer<const SharedAssetDepot> pDepot;
    std::unique_lock<std::mutex> lock;
  };

  // Maps asset keys to AssetEntry instances. This collection owns the asset
  // entries.
  std::unordered_map<TAssetKey, CesiumUtility::IntrusivePointer<AssetEntry>>
      _assets;

  // Maps asset pointers to AssetEntry instances. The values in this map refer
  // to instances owned by the _assets map.
  std::unordered_map<TAssetType*, AssetEntry*> _assetsByPointer;

  // List of assets that are being considered for deletion, in the order that
  // they became unused.
  CesiumUtility::DoublyLinkedList<AssetEntry, &AssetEntry::deletionListPointers>
      _deletionCandidates;

  // The total amount of memory used by all assets in the _deletionCandidates
  // list.
  int64_t _totalDeletionCandidateMemoryUsage;

  // Mutex serializing access to _assets, _assetsByPointer, _deletionCandidates,
  // and any AssetEntry owned by this depot.
  mutable std::mutex _mutex;

  // The factory used to create new AssetType instances.
  std::function<FactorySignature> _factory;

  // This instance keeps a reference to itself whenever it is managing active
  // assets, preventing it from being destroyed even if all other references to
  // it are dropped.
  CesiumUtility::IntrusivePointer<SharedAssetDepot<TAssetType, TAssetKey>>
      _pKeepAlive;
};

template <typename TAssetType, typename TAssetKey>
SharedAssetDepot<TAssetType, TAssetKey>::SharedAssetDepot(
    std::function<FactorySignature> factory)
    : _assets(),
      _assetsByPointer(),
      _deletionCandidates(),
      _totalDeletionCandidateMemoryUsage(0),
      _mutex(),
      _factory(std::move(factory)),
      _pKeepAlive(nullptr) {}

template <typename TAssetType, typename TAssetKey>
SharedAssetDepot<TAssetType, TAssetKey>::~SharedAssetDepot() {
  // Ideally, when the depot is destroyed, all the assets it owns would become
  // independent assets. But this is extremely difficult to manage in a
  // thread-safe manner.

  // Since we're in the destructor, we can be sure no one has a reference to
  // this instance anymore. That means that no other thread can be executing
  // `getOrCreate`, and no async asset creations are in progress.

  // However, if assets owned by this depot are still alive, then other
  // threads can still be calling addReference / releaseReference on some of
  // our assets even while we're running the depot's destructor. Which means
  // that we can end up in `markDeletionCandidate` at the same time the
  // destructor is running. And in fact it's possible for a `SharedAsset` with
  // especially poor timing to call into a `SharedAssetDepot` just after it is
  // destroyed.

  // To avoid this, we use the _pKeepAlive field to maintain an artificial
  // reference to this depot whenever it owns live assets. This should keep
  // this destructor from being called except when all of its assets are also
  // in the _deletionCandidates list.

  CESIUM_ASSERT(this->_assets.size() == this->_deletionCandidates.size());
}

template <typename TAssetType, typename TAssetKey>
SharedFuture<CesiumUtility::ResultPointer<TAssetType>>
SharedAssetDepot<TAssetType, TAssetKey>::getOrCreate(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const TAssetKey& assetKey) {
  // We need to take care here to avoid two assets starting to load before the
  // first asset has added an entry and set its maybePendingAsset field.
  LockHolder lock = this->lock();

  auto existingIt = this->_assets.find(assetKey);
  if (existingIt != this->_assets.end()) {
    // We've already loaded (or are loading) an asset with this ID - we can
    // just use that.
    const AssetEntry& entry = *existingIt->second;
    if (entry.maybePendingAsset) {
      // Asset is currently loading.
      return *entry.maybePendingAsset;
    } else {
      return asyncSystem.createResolvedFuture(entry.toResultUnderLock())
          .share();
    }
  }

  // Calling the factory function while holding the mutex unnecessarily
  // limits parallelism. It can even lead to a bug in the scenario where the
  // `thenInWorkerThread` continuation is invoked immediately in the current
  // thread, before `thenInWorkerThread` itself returns. That would result
  // in an attempt to lock the mutex recursively, which is not allowed.

  // So we jump through some hoops here to publish "this thread is working
  // on it", then unlock the mutex, and _then_ actually call the factory
  // function.
  Promise<void> promise = asyncSystem.createPromise<void>();

  // We haven't loaded or started to load this asset yet.
  // Let's do that now.
  CesiumUtility::IntrusivePointer<SharedAssetDepot<TAssetType, TAssetKey>>
      pDepot = this;
  CesiumUtility::IntrusivePointer<AssetEntry> pEntry = new AssetEntry(assetKey);

  auto future =
      promise.getFuture()
          .thenImmediately([pDepot, pEntry, asyncSystem, pAssetAccessor]() {
            return pDepot->_factory(asyncSystem, pAssetAccessor, pEntry->key);
          })
          .catchImmediately([](std::exception&& e) {
            return CesiumUtility::Result<
                CesiumUtility::IntrusivePointer<TAssetType>>(
                CesiumUtility::ErrorList::error(
                    std::string("Error creating asset: ") + e.what()));
          })
          .thenInWorkerThread(
              [pDepot,
               pEntry](CesiumUtility::Result<
                       CesiumUtility::IntrusivePointer<TAssetType>>&& result) {
                LockHolder lock = pDepot->lock();

                if (result.pValue) {
                  result.pValue->_pDepot = pDepot.get();
                  pDepot->_assetsByPointer[result.pValue.get()] = pEntry.get();
                }

                // Now that this asset is owned by the depot, we exclusively
                // control its lifetime with a std::unique_ptr.
                pEntry->pAsset =
                    std::unique_ptr<TAssetType>(result.pValue.get());
                pEntry->errorsAndWarnings = std::move(result.errors);
                pEntry->maybePendingAsset.reset();

                // The asset is initially live because we have an
                // IntrusivePointer to it right here. So make sure the depot
                // stays alive, too.
                pDepot->_pKeepAlive = pDepot;

                return pEntry->toResultUnderLock();
              });

  SharedFuture<CesiumUtility::ResultPointer<TAssetType>> sharedFuture =
      std::move(future).share();

  pEntry->maybePendingAsset = sharedFuture;

  [[maybe_unused]] bool added = this->_assets.emplace(assetKey, pEntry).second;

  // Should always be added successfully, because we checked above that the
  // asset key doesn't exist in the map yet.
  CESIUM_ASSERT(added);

  // Unlock the mutex and then call the factory function.
  lock.unlock();
  promise.resolve();

  return sharedFuture;
}

template <typename TAssetType, typename TAssetKey>
size_t SharedAssetDepot<TAssetType, TAssetKey>::getAssetCount() const {
  LockHolder lock = this->lock();
  return this->_assets.size();
}

template <typename TAssetType, typename TAssetKey>
size_t SharedAssetDepot<TAssetType, TAssetKey>::getActiveAssetCount() const {
  LockHolder lock = this->lock();
  return this->_assets.size() - this->_deletionCandidates.size();
}

template <typename TAssetType, typename TAssetKey>
size_t SharedAssetDepot<TAssetType, TAssetKey>::getInactiveAssetCount() const {
  LockHolder lock = this->lock();
  return this->_deletionCandidates.size();
}

template <typename TAssetType, typename TAssetKey>
int64_t
SharedAssetDepot<TAssetType, TAssetKey>::getInactiveAssetTotalSizeBytes()
    const {
  LockHolder lock = this->lock();
  return this->_totalDeletionCandidateMemoryUsage;
}

template <typename TAssetType, typename TAssetKey>
typename SharedAssetDepot<TAssetType, TAssetKey>::LockHolder
SharedAssetDepot<TAssetType, TAssetKey>::lock() const {
  return LockHolder{this};
}

template <typename TAssetType, typename TAssetKey>
void SharedAssetDepot<TAssetType, TAssetKey>::markDeletionCandidate(
    const TAssetType& asset,
    bool threadOwnsDepotLock) {
  if (threadOwnsDepotLock) {
    this->markDeletionCandidateUnderLock(asset);
  } else {
    LockHolder lock = this->lock();
    this->markDeletionCandidateUnderLock(asset);
  }
}

template <typename TAssetType, typename TAssetKey>
void SharedAssetDepot<TAssetType, TAssetKey>::markDeletionCandidateUnderLock(
    const TAssetType& asset) {
  // Verify that the reference count is still zero.
  // See: https://github.com/CesiumGS/cesium-native/issues/1073
  if (asset._referenceCount != 0) {
    return;
  }

  auto it = this->_assetsByPointer.find(const_cast<TAssetType*>(&asset));
  CESIUM_ASSERT(it != this->_assetsByPointer.end());
  if (it == this->_assetsByPointer.end()) {
    return;
  }

  CESIUM_ASSERT(it->second != nullptr);

  AssetEntry& entry = *it->second;
  entry.sizeInDeletionList = asset.getSizeBytes();
  this->_totalDeletionCandidateMemoryUsage += entry.sizeInDeletionList;

  this->_deletionCandidates.insertAtTail(entry);

  if (this->_totalDeletionCandidateMemoryUsage >
      this->inactiveAssetSizeLimitBytes) {
    // Delete the deletion candidates until we're below the limit.
    while (this->_deletionCandidates.size() > 0 &&
           this->_totalDeletionCandidateMemoryUsage >
               this->inactiveAssetSizeLimitBytes) {
      AssetEntry* pOldEntry = this->_deletionCandidates.head();
      this->_deletionCandidates.remove(*pOldEntry);

      this->_totalDeletionCandidateMemoryUsage -= pOldEntry->sizeInDeletionList;

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

  // If this depot is not managing any live assets, then we no longer need to
  // keep it alive.
  if (this->_assets.size() == this->_deletionCandidates.size()) {
    this->_pKeepAlive.reset();
  }
}

template <typename TAssetType, typename TAssetKey>
void SharedAssetDepot<TAssetType, TAssetKey>::unmarkDeletionCandidate(
    const TAssetType& asset,
    bool threadOwnsDepotLock) {
  if (threadOwnsDepotLock) {
    this->unmarkDeletionCandidateUnderLock(asset);
  } else {
    LockHolder lock = this->lock();
    this->unmarkDeletionCandidateUnderLock(asset);
  }
}

template <typename TAssetType, typename TAssetKey>
void SharedAssetDepot<TAssetType, TAssetKey>::unmarkDeletionCandidateUnderLock(
    const TAssetType& asset) {
  auto it = this->_assetsByPointer.find(const_cast<TAssetType*>(&asset));
  CESIUM_ASSERT(it != this->_assetsByPointer.end());
  if (it == this->_assetsByPointer.end()) {
    return;
  }

  CESIUM_ASSERT(it->second != nullptr);

  AssetEntry& entry = *it->second;
  bool isFound = this->_deletionCandidates.contains(entry);

  // The asset won't necessarily be found in the deletionCandidates set.
  // See: https://github.com/CesiumGS/cesium-native/issues/1073
  if (isFound) {
    this->_totalDeletionCandidateMemoryUsage -= entry.sizeInDeletionList;
    this->_deletionCandidates.remove(entry);
  }

  // This depot is now managing at least one live asset, so keep it alive.
  this->_pKeepAlive = this;
}

template <typename TAssetType, typename TAssetKey>
CesiumUtility::ResultPointer<TAssetType>
SharedAssetDepot<TAssetType, TAssetKey>::AssetEntry::toResultUnderLock() const {
  // This method is called while the calling thread already owns the depot
  // mutex. So we must take care not to lock it again, which could happen if
  // the asset is currently unreferenced and we naively create an
  // IntrusivePointer for it.
  CesiumUtility::IntrusivePointer<TAssetType> p = nullptr;
  if (pAsset) {
    pAsset->addReference(true);
    p = pAsset.get();
    pAsset->releaseReference(true);
  }
  return CesiumUtility::ResultPointer<TAssetType>(p, errorsAndWarnings);
}

template <typename TAssetType, typename TAssetKey>
SharedAssetDepot<TAssetType, TAssetKey>::LockHolder::LockHolder(
    const CesiumUtility::IntrusivePointer<const SharedAssetDepot>& pDepot_)
    : pDepot(pDepot_), lock(pDepot_->_mutex) {}

template <typename TAssetType, typename TAssetKey>
SharedAssetDepot<TAssetType, TAssetKey>::LockHolder::~LockHolder() = default;

template <typename TAssetType, typename TAssetKey>
void SharedAssetDepot<TAssetType, TAssetKey>::LockHolder::unlock() {
  this->lock.unlock();
}

} // namespace CesiumAsync
