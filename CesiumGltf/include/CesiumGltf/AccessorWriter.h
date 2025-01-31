#pragma once

#include <CesiumGltf/AccessorView.h>

namespace CesiumGltf {

/**
 * @brief Provides write access to an {@link AccessorView}.
 */
template <class T> class AccessorWriter final {
private:
  AccessorView<T> _accessor;

public:
  /**
   * @brief The type of the elements in the accessor.
   */
  typedef T value_type;

  AccessorWriter() : _accessor() {}

  /**
   * @brief Constructs a new instance from an {@link AccessorView}.
   */
  AccessorWriter(const AccessorView<T>& accessorView)
      : _accessor(accessorView) {}

  /** @copydoc AccessorView::AccessorView(const std::byte*, int64_t, int64_t,
   * int64_t) */
  AccessorWriter(std::byte* pData, int64_t stride, int64_t offset, int64_t size)
      : _accessor(pData, stride, offset, size) {}

  /** @copydoc AccessorView::AccessorView(const Model&,const Accessor&) */
  AccessorWriter(Model& model, const Accessor& accessor)
      : _accessor(model, accessor) {}

  /** @copydoc AccessorView::AccessorView(const Model&,int32_t) */
  AccessorWriter(Model& model, int32_t accessorIndex) noexcept
      : _accessor(model, accessorIndex) {}

  /** @copydoc AccessorView::operator[]() */
  const T& operator[](int64_t i) const { return this->_accessor[i]; }

  /** @copydoc AccessorView::operator[]() */
  T& operator[](int64_t i) { return const_cast<T&>(this->_accessor[i]); }

  /** @copydoc AccessorView::size */
  int64_t size() const noexcept { return this->_accessor.size(); }

  /**
   * @brief Gets the status of this accessor writer.
   *
   * Indicates whether the writer accurately reflects the accessor's data, or
   * whether an error occurred.
   */
  AccessorViewStatus status() const noexcept {
    return this->_accessor.status();
  }

  /** @copydoc AccessorView::stride */
  int64_t stride() const noexcept { return this->_accessor.stride(); }

  /** @copydoc AccessorView::offset */
  int64_t offset() const noexcept { return this->_accessor.offset(); }

  /** @copydoc AccessorView::data */
  std::byte* data() noexcept {
    return const_cast<std::byte*>(this->_accessor.data());
  }
};

} // namespace CesiumGltf
