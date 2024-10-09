#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace CesiumGltf {

template <typename T> class SharedAsset;

/**
 * A depot for {@link SharedAsset} instances, which are potentially shared between multiple objects.
 * @tparam AssetType The type of asset stored in this depot. This should usually
 * be derived from {@link SharedAsset}.
 */
template <typename AssetType>
class SharedAssetDepot : public CesiumUtility::ReferenceCountedThreadSafe<
                             SharedAssetDepot<AssetType>> {
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
   * Default is 100MB.
   */
  int64_t staleAssetSizeLimit = 1000 * 1000 * 100;

  SharedAssetDepot() = default;

  ~SharedAssetDepot() {
    // It's possible the assets will outlive the depot, if they're still in use.
    std::lock_guard lock(this->assetsMutex);

    for (auto& pair : this->assets) {
      // Transfer ownership to the reference counting system.
      CesiumUtility::IntrusivePointer<AssetType> pCounted =
          pair.second.release();
      pCounted->_pDepot = nullptr;
      pCounted->_uniqueAssetId.clear();
    }
  }

  /**
   * Stores the AssetType in this depot and returns a reference to it,
   * or returns the existing asset if present.
   */
  CesiumUtility::IntrusivePointer<AssetType> store(
      const std::string& assetId,
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
    std::lock_guard lock(this->assetsMutex);

    auto findIt = this->assets.find(assetId);
    if (findIt != this->assets.end()) {
      // This asset ID already exists in the depot, so we can't add this asset.
      return findIt->second.get();
    }

    pAsset->_pDepot = this;
    pAsset->_uniqueAssetId = assetId;

    // Now that this asset is owned by the depot, we exclusively
    // control its lifetime with a std::unique_ptr.
    std::unique_ptr<AssetType> pOwnedAsset(pAsset.get());

    auto [addIt, added] = this->assets.emplace(assetId, std::move(pOwnedAsset));

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
  template <typename Factory>
  CesiumAsync::SharedFuture<CesiumUtility::IntrusivePointer<AssetType>>
  getOrFetch(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const Factory& factory,
      const std::string& uri,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
    // We need to avoid:
    // - Two assets starting to load before the first asset has updated the
    //   pendingAssets map
    // - An asset starting to load after the previous load has been removed from
    //   the pendingAssets map, but before the completed asset has been added to
    //   the assets map.
    std::lock_guard lock(this->assetsMutex);

    auto existingIt = this->assets.find(uri);
    if (existingIt != this->assets.end()) {
      // We've already loaded an asset with this ID - we can just use that.
      return asyncSystem
          .createResolvedFuture(CesiumUtility::IntrusivePointer<AssetType>(
              existingIt->second.get()))
          .share();
    }

    auto pendingIt = this->pendingAssets.find(uri);
    if (pendingIt != this->pendingAssets.end()) {
      // Return the existing future - the caller can chain off of it.
      return pendingIt->second;
    }

    // We haven't loaded or started to load this asset yet.
    // Let's do that now.
    CesiumAsync::Future<CesiumUtility::IntrusivePointer<AssetType>> future =
        pAssetAccessor->get(asyncSystem, uri, headers)
            .thenInWorkerThread(
                [factory = std::move(factory)](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest)
                    -> CesiumUtility::IntrusivePointer<AssetType> {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  if (!pResponse) {
                    return nullptr;
                  }

                  return factory.createFrom(pResponse->data());
                })
            // Do this in main thread since we're messing with the collections.
            .thenInMainThread(
                [uri,
                 this](CesiumUtility::IntrusivePointer<AssetType>&& pResult)
                    -> CesiumUtility::IntrusivePointer<AssetType> {
                  // Get rid of our future.
                  this->pendingAssets.erase(uri);

                  // Store the new asset in the depot.
                  return this->store(uri, pResult);
                });

    auto [it, ok] = this->pendingAssets.emplace(uri, std::move(future).share());
    if (!ok) {
      return asyncSystem
          .createResolvedFuture<CesiumUtility::IntrusivePointer<AssetType>>(
              nullptr)
          .share();
    }

    return it->second;
  }

  /**
   * @brief Returns the total number of distinct assets contained in this depot.
   */
  size_t getDistinctCount() const { return this->assets.size(); }

  /**
   * @brief Returns the number of active references to assets in this depot.
   */
  size_t getUsageCount() const {
    size_t count = 0;
    for (const auto& [key, item] : assets) {
      count += item->_referenceCount;
    }
    return count;
  }

  size_t getDeletionCandidateCount() const {
    std::lock_guard lock(this->deletionCandidatesMutex);
    return this->deletionCandidates.size();
  }

  int64_t getDeletionCandidateTotalSizeBytes() const {
    return this->totalDeletionCandidateMemoryUsage;
  }

private:
  // Disable copy
  void operator=(const SharedAssetDepot<AssetType>& other) = delete;

  /**
   * Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void markDeletionCandidate(const AssetType* pAsset) {
    std::lock_guard lock(this->deletionCandidatesMutex);
    this->deletionCandidates.push_back(const_cast<AssetType*>(pAsset));
    this->totalDeletionCandidateMemoryUsage += pAsset->getSizeBytes();

    if (this->totalDeletionCandidateMemoryUsage > this->staleAssetSizeLimit) {
      std::lock_guard assetsLock(this->assetsMutex);
      while (!this->deletionCandidates.empty() &&
             this->totalDeletionCandidateMemoryUsage >
                 this->staleAssetSizeLimit) {
        const AssetType* pOldAsset = this->deletionCandidates.front();
        this->deletionCandidates.pop_front();
        CESIUM_ASSERT(pOldAsset->_referenceCount == 0);
        this->assets.erase(pOldAsset->getUniqueAssetId());
        this->totalDeletionCandidateMemoryUsage -= pOldAsset->getSizeBytes();
      }
    }
  }

  /**
   * Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void unmarkDeletionCandidate(const AssetType* pAsset) {
    std::lock_guard lock(this->deletionCandidatesMutex);
    const std::string& assetId = pAsset->getUniqueAssetId();
    for (auto it = this->deletionCandidates.begin();
         it != this->deletionCandidates.end();
         ++it) {
      if ((*it)->getUniqueAssetId() == assetId) {
        this->deletionCandidates.erase(it);
        this->totalDeletionCandidateMemoryUsage -= (*it)->getSizeBytes();
        break;
      }
    }
  }

  // Assets that have a unique ID that can be used to de-duplicate them.
  std::unordered_map<std::string, std::unique_ptr<AssetType>> assets;
  // Futures for assets that still aren't loaded yet.
  std::unordered_map<
      std::string,
      CesiumAsync::SharedFuture<CesiumUtility::IntrusivePointer<AssetType>>>
      pendingAssets;
  // Mutex for checking or editing the pendingAssets map
  std::mutex assetsMutex;

  // List of assets that are being considered for deletion, in the order that
  // they were added.
  std::list<AssetType*> deletionCandidates;
  // The total amount of memory used by all assets in the deletionCandidates
  // list.
  std::atomic<int64_t> totalDeletionCandidateMemoryUsage;
  // Mutex for modifying the deletionCandidates list.
  mutable std::mutex deletionCandidatesMutex;

  friend class SharedAsset<AssetType>;
};

} // namespace CesiumGltf
