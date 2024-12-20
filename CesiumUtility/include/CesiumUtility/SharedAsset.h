#pragma once

#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/IDepotOwningAsset.h>
#include <CesiumUtility/Library.h>

#include <atomic>

namespace CesiumAsync {
template <typename TAssetType, typename TAssetKey> class SharedAssetDepot;
}

namespace CesiumUtility {

/**
 * @brief An asset that is potentially shared between multiple objects, such as
 * an image shared between multiple glTF models. This is intended to be the base
 * class for such assets.
 *
 * The lifetime of instances of this class should be managed by reference
 * counting with {@link IntrusivePointer}.
 *
 * @tparam T The type that is _deriving_ from this class. For example, you
 * should declare your class as
 * `class MyClass : public SharedAsset<MyClass> { ... };`
 *
 * @remarks @parblock
 * A `SharedAsset` can be in one of three possible states:
 *
 * **Independent Asset**
 * An independent asset isn't affiliated with an asset depot at all.
 * Its lifetime is controlled exclusively by IntrusivePointer / reference
 * counting. When the asset's reference count goes to zero, it deletes itself.
 * An independent asset's {@link getDepot} returns nullptr.
 *
 * **Active Depot Asset**
 * This is an asset that is owned by an asset depot and that is in use, meaning
 * it has a reference count greater than zero. The asset depot owns the asset
 * via an `std::unique_ptr`, not via adding to the reference count. So when the
 * reference count goes to zero, only the asset depot itself still has a
 * reference to it, so it becomes an inactive depot asset.
 *
 * **Inactive Depot Asset**
 * This is also an asset that is owned by the asset depot, but there are no
 * other references to it (it has a reference count of zero). It is found in the
 * asset depot's `deletionCandidates` list. When a reference to it is added, it
 * is removed from `deletionCandidates` and it becomes an active depot asset.
 * @endparblock
 */
template <typename T>
class CESIUMUTILITY_API SharedAsset : public CesiumUtility::ExtensibleObject {
public:
  /**
   * @brief Adds a counted reference to this object. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void addReference() const noexcept { this->addReference(false); }

  /**
   * @brief Removes a counted reference from this object. When the last
   * reference is removed, this method will delete this instance. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void releaseReference() const noexcept { this->releaseReference(false); }

  /**
   * @brief Gets the shared asset depot that owns this asset, or nullptr if this
   * asset is independent of an asset depot.
   */
  const IDepotOwningAsset<T>* getDepot() const { return this->_pDepot; }

  /**
   * @brief Gets the shared asset depot that owns this asset, or nullptr if this
   * asset is independent of an asset depot.
   */
  IDepotOwningAsset<T>* getDepot() { return this->_pDepot; }

protected:
  SharedAsset() = default;
  ~SharedAsset() { CESIUM_ASSERT(this->_referenceCount == 0); }

  /**
   * Assets can be copied, but the fresh instance has no references and is not
   * in the asset depot.
   */
  SharedAsset(const SharedAsset& rhs)
      : ExtensibleObject(rhs), _referenceCount(0), _pDepot(nullptr) {}

  /**
   * After a move construction, the content of the asset is moved to the new
   * instance, but the asset depot still references the old instance.
   */
  SharedAsset(SharedAsset&& rhs)
      : ExtensibleObject(std::move(rhs)),
        _referenceCount(0),
        _pDepot(nullptr) {}

  /**
   * Assignment does not affect the asset's relationship with the depot, but is
   * useful to assign the data in derived classes.
   */
  SharedAsset& operator=(const SharedAsset& rhs) {
    CesiumUtility::ExtensibleObject::operator=(rhs);
    return *this;
  }

  /**
   * Assignment does not affect the asset's relationship with the depot, but is
   * useful to assign the data in derived classes.
   */
  SharedAsset& operator=(SharedAsset&& rhs) {
    CesiumUtility::ExtensibleObject::operator=(std::move(rhs));
    return *this;
  }

private:
  void addReference(bool threadOwnsDepotLock) const noexcept {
    const int32_t prevReferences = this->_referenceCount++;
    if (this->_pDepot && prevReferences <= 0) {
      this->_pDepot->unmarkDeletionCandidate(
          *static_cast<const T*>(this),
          threadOwnsDepotLock);
    }
  }

  void releaseReference(bool threadOwnsDepotLock) const noexcept {
    CESIUM_ASSERT(this->_referenceCount > 0);
    const int32_t references = --this->_referenceCount;
    if (references == 0) {
      IDepotOwningAsset<T>* pDepot = this->_pDepot;
      if (pDepot) {
        // Let the depot manage this object's lifetime.
        pDepot->markDeletionCandidate(
            *static_cast<const T*>(this),
            threadOwnsDepotLock);
      } else {
        // No depot, so destroy this object directly.
        delete static_cast<const T*>(this);
      }
    }
  }

  mutable std::atomic<std::int32_t> _referenceCount{0};
  IDepotOwningAsset<T>* _pDepot{nullptr};

  // To allow the depot to modify _pDepot.
  template <typename TAssetType, typename TAssetKey>
  friend class CesiumAsync::SharedAssetDepot;
};

} // namespace CesiumUtility
