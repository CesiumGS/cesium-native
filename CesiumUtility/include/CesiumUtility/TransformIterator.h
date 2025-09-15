#pragma once

#include <iterator>
#include <type_traits>

namespace CesiumUtility {

template <typename TTransformFunction, typename TIterator>
class TransformIterator {
public:
  using iterator_category =
      typename std::iterator_traits<TIterator>::iterator_category;
  using value_type = typename std::invoke_result_t<
      TTransformFunction,
      typename std::iterator_traits<TIterator>::reference>;
  using difference_type =
      typename std::iterator_traits<TIterator>::difference_type;
  using reference = value_type;

  // We can't directly return a pointer to the transformed value from our
  // operator->(), because that's likely to be a temporary. So instead, we wrap
  // it in an instance of this class and rely on C++'s automatic repeated
  // application of operator->().
  struct pointer {
    value_type value;
    value_type* operator->() { return &value; }
  };

  TransformIterator(TTransformFunction transformFunction, TIterator iterator)
      : _iterator(iterator), _transformFunction(transformFunction) {}

  value_type operator*() const {
    return this->_transformFunction(*this->_iterator);
  }

  pointer operator->() const {
    return pointer{this->_transformFunction(*this->_iterator)};
  }

  TransformIterator& operator++() {
    ++this->_iterator;
    return *this;
  }

  TransformIterator operator++(int) {
    TransformIterator temp = *this;
    ++(*this);
    return temp;
  }

  bool operator==(const TransformIterator& rhs) const {
    return this->_iterator == rhs._iterator;
  }

  bool operator!=(const TransformIterator& other) const {
    return this->_iterator != other._iterator;
  }

private:
  TIterator _iterator;

  // We could get fancy and use the empty base class optimization to make this
  // iterator type smaller in the (common) case that the transform function has
  // zero size. But let's keep it simple and just store it as a field for now.
  TTransformFunction _transformFunction;
};

} // namespace CesiumUtility
