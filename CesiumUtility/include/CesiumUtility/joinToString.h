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
  return std::accumulate(
      begin,
      end,
      std::string(),
      [&separator](const std::string& acc, const std::string& element) {
        if (!acc.empty()) {
          return acc + separator + element;
        } else {
          return element;
        }
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
std::string joinToString(TCollection collection, const std::string& separator) {
  return std::accumulate(
      collection.cbegin(),
      collection.cend(),
      std::string(),
      [&separator](const std::string& acc, const std::string& element) {
        if (!acc.empty()) {
          return acc + separator + element;
        } else {
          return element;
        }
      });
}
} // namespace CesiumUtility
