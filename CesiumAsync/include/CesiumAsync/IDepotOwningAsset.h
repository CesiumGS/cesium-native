#pragma once

namespace CesiumAsync {

/**
 * @brief An interface representing the depot that owns a {@link SharedAsset}.
 *
 * {@link SharedAsset} has a pointer to the asset depot that owns it using this
 * interface, rather than a complete {@link SharedAssetDepot}, in order to
 * "erase" the type of the asset key. This allows SharedAsset to be templatized
 * only on the asset type, not on the asset key type.
 */
template <typename TAssetType> class IDepotOwningAsset {
public:
  virtual ~IDepotOwningAsset() {}
  virtual void markDeletionCandidate(const TAssetType& asset) = 0;
  virtual void unmarkDeletionCandidate(const TAssetType& asset) = 0;
};

} // namespace CesiumAsync
