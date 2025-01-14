#pragma once

namespace CesiumUtility {

/**
 * @brief An interface representing the depot that owns a {@link SharedAsset}.
 * This interface is an implementation detail of the shared asset system and
 * should not be used directly.
 *
 * {@link SharedAsset} has a pointer to the asset depot that owns it using this
 * interface, rather than a complete {@link CesiumAsync::SharedAssetDepot}, in
 * order to "erase" the type of the asset key. This allows SharedAsset to be
 * templatized only on the asset type, not on the asset key type.
 */
template <typename TAssetType> class IDepotOwningAsset {
public:
  virtual ~IDepotOwningAsset() = default;

  /**
   * @brief Marks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   *
   * @param asset The asset to mark for deletion.
   * @param threadOwnsDepotLock True if the calling thread already owns the
   * depot lock; otherwise, false.
   */
  virtual void
  markDeletionCandidate(const TAssetType& asset, bool threadOwnsDepotLock) = 0;

  /**
   * @brief Unmarks the given asset as a candidate for deletion.
   * Should only be called by {@link SharedAsset}. May be called from any thread.
   *
   * @param asset The asset to unmark for deletion.
   * @param threadOwnsDepotLock True if the calling thread already owns the
   * depot lock; otherwise, false.
   */
  virtual void unmarkDeletionCandidate(
      const TAssetType& asset,
      bool threadOwnsDepotLock) = 0;
};

} // namespace CesiumUtility
