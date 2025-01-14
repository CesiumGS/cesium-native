#pragma once

#include <numeric>
#include <string>

namespace CesiumUtility {
/**
 * @brief Joins multiple elements together into a string, separated by a given
 * separator.
 *
 * @tparam TIterator The type of the collection iterator.
 * @param begin An iterator referring to the first element to join.
 * @param end An iterator referring to one past the last element to join.
 * @param separator The string to use to separate successive elements.
 * @return The joined string.
 */
template <class TIterator>
std::string
joinToString(TIterator begin, TIterator end, const std::string& separator) {
  if (begin == end)
    return std::string();

  std::string first = *begin;

  return std::accumulate(
      ++begin,
      end,
      std::move(first),
      [&separator](const std::string& acc, const std::string& element) {
        return acc + separator + element;
      });
}

/**
 * @brief Joins multiple elements together into a string, separated by a given
 * separator.
 *
 * @tparam TIterator The type of the collection iterator.
 * @param collection The collection of elements to be joined.
 * @param separator The string to use to separate successive elements.
 * @return The joined string.
 */
template <class TCollection>
std::string
joinToString(const TCollection& collection, const std::string& separator) {
  return joinToString(collection.cbegin(), collection.cend(), separator);
}
} // namespace CesiumUtility
