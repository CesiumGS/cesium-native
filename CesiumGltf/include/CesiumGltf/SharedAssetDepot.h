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

  /**
   * Stores the AssetType in this depot and returns a reference to it,
   * or returns the existing asset if present.
   */
  CesiumUtility::IntrusivePointer<AssetType> store(
      std::string& assetId,
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
    auto [newIt, added] = this->assets.try_emplace(assetId, pAsset);
    return newIt->second;
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
          .createResolvedFuture(
              CesiumUtility::IntrusivePointer<AssetType>(existingIt->second))
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
                 this,
                 pAssets = &this->assets,
                 pPendingAssetsMutex = &this->assetsMutex,
                 pPendingAssets = &this->pendingAssets](
                    CesiumUtility::IntrusivePointer<AssetType>&& pResult)
                    -> CesiumUtility::IntrusivePointer<AssetType> {
                  std::lock_guard lock(*pPendingAssetsMutex);

                  // Get rid of our future.
                  pPendingAssets->erase(uri);

                  if (pResult) {
                    pResult->_pDepot = this;
                    pResult->_uniqueAssetId = uri;

                    auto [it, ok] = pAssets->emplace(uri, pResult);
                    if (!ok) {
                      return nullptr;
                    }

                    return it->second;
                  }

                  return nullptr;
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

  /**
   * @brief Obtains statistics on the number of assets currently listed as
   * candidates for deletion, along with the average number of seconds that
   * they've been pending deletion.
   * @param outAverageAge The average time in seconds that the current deletion
   * candidates have spent pending deletion.
   * @param outCount The number of current deletion candidates.
   */
  void getDeletionStats(float& outAverageAge, size_t& outCount) const {
    size_t count = 0;
    float total = 0;

    std::lock_guard lock(this->deletionCandidatesMutex);
    for (auto& [asset, age] : this->deletionCandidates) {
      total += age;
      count++;
    }

    outAverageAge = count > 0 ? total / count : 0;
    outCount = count;
  }

private:
  // Disable copy
  void operator=(const SharedAssetDepot<AssetType>& other) = delete;

  /**
   * Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void markDeletionCandidate(
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
    std::lock_guard lock(this->deletionCandidatesMutex);
    this->deletionCandidates.push_back(pAsset);
    this->totalDeletionCandidateMemoryUsage += pAsset->getSizeBytes();

    if (this->totalDeletionCandidateMemoryUsage > this->staleAssetSizeLimit) {
      std::lock_guard assetsLock(this->assetsMutex);
      while (!this->deletionCandidates.empty() &&
             this->totalDeletionCandidateMemoryUsage >
                 this->staleAssetSizeLimit) {
        const CesiumUtility::IntrusivePointer<AssetType>& pOldAsset =
            this->deletionCandidates.front();
        this->deletionCandidates.pop_front();
        this->assets.erase(pOldAsset->getUniqueAssetId());
        this->totalDeletionCandidateMemoryUsage -= pOldAsset->getSizeBytes();
      }
    }
  }

  /**
   * Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void unmarkDeletionCandidate(
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
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
  std::unordered_map<std::string, CesiumUtility::IntrusivePointer<AssetType>>
      assets;
  // Futures for assets that still aren't loaded yet.
  std::unordered_map<
      std::string,
      CesiumAsync::SharedFuture<CesiumUtility::IntrusivePointer<AssetType>>>
      pendingAssets;
  // Mutex for checking or editing the pendingAssets map
  std::mutex assetsMutex;

  // List of assets that are being considered for deletion, in the order that
  // they were added.
  std::list<CesiumUtility::IntrusivePointer<AssetType>> deletionCandidates;
  // The total amount of memory used by all assets in the deletionCandidates
  // list.
  int64_t totalDeletionCandidateMemoryUsage;
  // Mutex for modifying the deletionCandidates list.
  mutable std::mutex deletionCandidatesMutex;

  friend class SharedAsset<AssetType>;
};

} // namespace CesiumGltf
