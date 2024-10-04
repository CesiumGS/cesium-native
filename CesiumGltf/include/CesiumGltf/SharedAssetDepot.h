#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <chrono>
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
   * @brief Number of seconds an asset can remain unused before it's deleted.
   */
  float assetDeletionThreshold = 60.0f;

  SharedAssetDepot() {
    this->lastDeletionTick = std::chrono::steady_clock::now();
  }

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
    // - Two assets starting loading before the first asset has updated the
    //   pendingAssets map
    // - An asset starting to load after the previous load has been removed from
    //   the pendingAssets map, but before the completed asset has been added to
    //   the assets map.
    std::lock_guard lock(assetsMutex);

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
   * @brief Should be called every frame to handle deletion of assets that
   * haven't been used in a while.
   */
  void deletionTick() {
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::duration<float> delta =
        std::chrono::duration<float>(now - this->lastDeletionTick);

    std::vector<std::string> toDelete;

    std::lock_guard lock(this->deletionCandidatesMutex);
    for (auto& [hash, age] : this->deletionCandidates) {
      age += delta.count();

      if (age >= this->assetDeletionThreshold) {
        toDelete.push_back(hash);
      }
    }

    std::lock_guard assetsLock(this->assetsMutex);
    for (std::string& hash : toDelete) {
      this->deletionCandidates.erase(hash);
      this->assets.erase(hash);
    }

    this->lastDeletionTick = now;
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
    for (auto& [key, item] : assets) {
      count += item->counter;
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
  // Disable copy and move
  void operator=(const SharedAssetDepot<AssetType>& other) = delete;

  /**
   * Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}.
   */
  void markDeletionCandidate(
      const std::string& hash,
      const CesiumUtility::IntrusivePointer<AssetType>& pAsset) {
    std::lock_guard lock(this->deletionCandidatesMutex);
    this->deletionCandidates.emplace(hash, 0.0f);
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

  // Maps assets that are being considered for deletion to the time that they
  // began to be considered for deletion.
  std::unordered_map<std::string, float> deletionCandidates;
  // Mutex for modifying the deletionCandidates map.
  mutable std::mutex deletionCandidatesMutex;
  std::chrono::steady_clock::time_point lastDeletionTick;

  friend class SharedAsset<AssetType>;
};

} // namespace CesiumGltf
