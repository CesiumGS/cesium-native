#pragma once

#include <iterator>
#include <type_traits>

namespace CesiumUtility {

/**
 * @brief An iterator that wraps another iterator and applies a transformation
 * function to each element as it is accessed.
 */
template <typename TTransformFunction, typename TIterator>
class TransformIterator {
public:
  /**
   * @brief The iterator category tag denoting this iterator has the same
   * category as the underlying iterator.
   */
  using iterator_category =
      typename std::iterator_traits<TIterator>::iterator_category;

  /**
   * @brief The type of value that is being iterated over, which is the result
   * of the transformation function applied to the underlying iterator's value.
   */
  using value_type = typename std::invoke_result_t<
      TTransformFunction,
      typename std::iterator_traits<TIterator>::reference>;

  /**
   * @brief The type used to identify distance between iterators, which for this
   * iterator is the same as the underlying iterator's `different_type`.
   */
  using difference_type =
      typename std::iterator_traits<TIterator>::difference_type;

  /**
   * @brief A pointer to the type being iterated over.
   *
   * We can't directly return a pointer to the transformed value from our
   * operator->(), because that's likely to be a temporary. So instead, we wrap
   * it in an instance of this class and rely on C++'s automatic repeated
   * application of operator->().
   */
  struct pointer {
    /**
     * @brief The value pointed to by the pointer.
     */
    value_type value;

    /**
     * @brief Returns a pointer to the value.
     */
    value_type* operator->() { return &value; }
  };

  /**
   * @brief Creates a new instance.
   *
   * @param transformFunction The function to apply to each element as it is
   * accessed. This function must be callable with a single argument of the type
   * being iterated over.
   * @param iterator The underlying iterator to wrap.
   */
  TransformIterator(TTransformFunction transformFunction, TIterator iterator)
      : _iterator(iterator), _transformFunction(transformFunction) {}

  /**
   * @brief Returns the current item being iterated. This applies the
   * transformation function to the current item of the underlying iterator.
   */
  value_type operator*() const {
    return this->_transformFunction(*this->_iterator);
  }

  /**
   * @brief Returns a pointer to the current item being iterated. This applies
   * the transformation function to the current item of the underlying iterator.
   */
  pointer operator->() const {
    return pointer{this->_transformFunction(*this->_iterator)};
  }

  /**
   * @brief Advances the iterator to the next item (pre-incrementing).
   */
  TransformIterator& operator++() {
    ++this->_iterator;
    return *this;
  }

  /**
   * @brief Advances the iterator to the next item (post-incrementing).
   */
  TransformIterator operator++(int) {
    TransformIterator temp = *this;
    ++(*this);
    return temp;
  }

  /**
   * @brief Checks if two iterators are at the same item.
   */
  bool operator==(const TransformIterator& rhs) const {
    return this->_iterator == rhs._iterator;
  }

  /**
   * @brief Checks if two iterators are NOT at the same item.
   */
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
