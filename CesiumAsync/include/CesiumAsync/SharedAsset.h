#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <atomic>

namespace CesiumAsync {

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
 */
template <typename T>
class CESIUMASYNC_API SharedAsset : public CesiumUtility::ExtensibleObject {
public:
  // Assets can be copied, but the fresh instance has no references and is not
  // in the asset depot.
  SharedAsset(const SharedAsset& rhs)
      : ExtensibleObject(rhs),
        _referenceCount(0),
        _pDepot(nullptr),
        _uniqueAssetId() {}

  // After a move construction, the content of the asset is moved to the new
  // instance, but the asset depot still references the old instance.
  SharedAsset(SharedAsset&& rhs)
      : ExtensibleObject(std::move(rhs)),
        _referenceCount(0),
        _pDepot(nullptr),
        _uniqueAssetId() {}

  // Assignment does not affect the asset's relationship with the depot, but is
  // useful to assign the data in derived classes.
  SharedAsset& operator=(const SharedAsset& rhs) {
    CesiumUtility::ExtensibleObject::operator=(rhs);
    return *this;
  }

  SharedAsset& operator=(SharedAsset&& rhs) {
    CesiumUtility::ExtensibleObject::operator=(std::move(rhs));
    return *this;
  }

  /**
   * @brief Adds a counted reference to this object. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void addReference() const /*noexcept*/ {
    const int32_t prevReferences = this->_referenceCount++;
    if (this->_pDepot && prevReferences <= 0) {
      this->_pDepot->unmarkDeletionCandidate(static_cast<const T*>(this));
    }
  }

  /**
   * @brief Removes a counted reference from this object. When the last
   * reference is removed, this method will delete this instance. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void releaseReference() const /*noexcept*/ {
    CESIUM_ASSERT(this->_referenceCount > 0);
    const int32_t references = --this->_referenceCount;
    if (references == 0) {
      SharedAssetDepot<T>* pDepot = this->_pDepot;
      if (pDepot) {
        // Let the depot manage this object's lifetime.
        pDepot->markDeletionCandidate(static_cast<const T*>(this));
      } else {
        // No depot, so destroy this object directly.
        delete static_cast<const T*>(this);
      }
    }
  }

  /**
   * @brief Gets the shared asset depot that owns this asset, or nullptr if this
   * asset is independent of an asset depot.
   */
  const SharedAssetDepot<T>* getDepot() const { return this->_pDepot; }

  /**
   * @brief Gets the shared asset depot that owns this asset, or nullptr if this
   * asset is independent of an asset depot.
   */
  SharedAssetDepot<T>* getDepot() { return this->_pDepot; }

  /**
   * @brief Gets the unique ID of this asset, if it {@link isShareable}.
   *
   * If this asset is not shareable, this method will return an empty string.
   */
  const std::string& getUniqueAssetId() const { return this->_uniqueAssetId; }

protected:
  SharedAsset() = default;
  ~SharedAsset() { CESIUM_ASSERT(this->_referenceCount == 0); }

private:
  mutable std::atomic<std::int32_t> _referenceCount{0};
  SharedAssetDepot<T>* _pDepot{nullptr};
  std::string _uniqueAssetId{};

  // The size of this asset when it was counted by the depot. This is stored so
  // that the exact same size can be subtracted later.
  int64_t _sizeInDepot{0};

  // To allow the depot to modify _pDepot, _uniqueAssetId, and _sizeInDepot.
  friend class SharedAssetDepot<T>;
};

} // namespace CesiumAsync
