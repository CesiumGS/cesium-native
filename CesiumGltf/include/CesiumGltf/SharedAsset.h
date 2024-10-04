#pragma once

#include <CesiumGltf/SharedAssetDepot.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <atomic>

namespace CesiumGltf {

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
class CESIUMGLTF_API SharedAsset : public CesiumUtility::ExtensibleObject {
public:
  SharedAsset() = default;
  ~SharedAsset() { CESIUM_ASSERT(this->_referenceCount == 0); }

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
      this->_pDepot->unmarkDeletionCandidate(
          const_cast<T*>(static_cast<const T*>(this)));
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
      CesiumUtility::IntrusivePointer<SharedAssetDepot<T>> pDepot =
          this->_pDepot;
      if (pDepot) {
        // Let the depot manage this object's lifetime.
        pDepot->markDeletionCandidate(
            const_cast<T*>(static_cast<const T*>(this)));
      } else {
        // No depot, so destroy this object directly.
        delete static_cast<const T*>(this);
      }
    }
  }

  /**
   * @brief Determines if this image is shareable because it is managed by an
   * asset depot. An image that is not shareable can be understood to be
   * exclusively owned by, for example, the glTF that references it. If it is
   * shareable, then potentially multiple glTFs reference it.
   *
   * An example of a non-shareable asset is an image embedded in a Binary glTF
   * (GLB) buffer. An example of a shareable asset is an image referenced in a
   * glTF by URI.
   */
  bool isShareable() const { return this->_pDepot != nullptr; }

  /**
   * The number of bytes of memory usage that this asset takes up.
   * This is used for deletion logic by the {@link SharedAssetDepot}.
   */
  virtual int64_t getSizeBytes() const = 0;

private:
  const std::string& getUniqueAssetId() const { return this->_uniqueAssetId; }

  mutable std::atomic<std::int32_t> _referenceCount{0};
  CesiumUtility::IntrusivePointer<SharedAssetDepot<T>> _pDepot;
  std::string _uniqueAssetId;

  // To allow the depot to modify _pDepot and _uniqueAssetId.
  friend class SharedAssetDepot<T>;
};

} // namespace CesiumGltf
