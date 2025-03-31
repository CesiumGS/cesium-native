#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>

#include <rapidjson/document.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace CesiumITwinClient {

class Connection;

/**
 * @brief Allows access to a set of resources from a paginated list.
 *
 * Many endpoints in the iTwin API return links to the previous and next pages,
 * if any, in their results. This class allows these links to be easily
 * traversed.
 */
template <typename T> class PagedList {
public:
  /**
   * @brief Callback used to obtain a page of results from a URL.
   */
  using PageOperation = std::function<CesiumAsync::Future<
      CesiumUtility::Result<PagedList<T>>>(Connection&, const std::string&)>;

  /**
   * @brief Creates a new `PagedList` from a set of items, an iTwin API
   * response, and a callback to retrieve more items.
   *
   * @param doc The JSON document obtained from the iTwin API.
   * @param items The parsed items that should be returned to the user.
   * @param operation Callback that can obtain a new page of results from a
   * given URL.
   */
  PagedList(
      rapidjson::Document& doc,
      std::vector<T>&& items,
      PageOperation&& operation)
      : _operation(std::move(operation)), _items(std::move(items)) {
    const auto& linksMember = doc.FindMember("_links");
    if (linksMember != doc.MemberEnd() && linksMember->value.IsObject()) {
      const auto& selfMember = linksMember->value.FindMember("self");
      if (selfMember != doc.MemberEnd() && selfMember->value.IsObject()) {
        this->_selfUrl = CesiumUtility::JsonHelpers::getStringOrDefault(
            selfMember->value,
            "href",
            "");
      }

      const auto& nextMember = linksMember->value.FindMember("next");
      if (nextMember != doc.MemberEnd() && nextMember->value.IsObject()) {
        this->_nextUrl = CesiumUtility::JsonHelpers::getStringOrDefault(
            nextMember->value,
            "href",
            "");
      }

      const auto& prevMember = linksMember->value.FindMember("prev");
      if (prevMember != doc.MemberEnd() && prevMember->value.IsObject()) {
        this->_prevUrl = CesiumUtility::JsonHelpers::getStringOrDefault(
            prevMember->value,
            "href",
            "");
      }
    }
  }

  /**
   * @brief Returns the contained item at `index`.
   */
  T& operator[](size_t index) { return _items[index]; }

  /**
   * @brief Returns the contained item at `index`.
   */
  const T& operator[](size_t index) const { return _items[index]; }

  /**
   * @brief Returns the number of contained items.
   */
  size_t size() const { return _items.size(); }

  /**
   * @brief The `begin` iterator of the underlying vector.
   */
  auto begin() { return _items.begin(); }
  /**
   * @brief The `begin` iterator of the underlying vector.
   */
  auto begin() const { return _items.begin(); }

  /**
   * @brief The `end` iterator of the underlying vector.
   */
  auto end() { return _items.end(); }
  /**
   * @brief The `end` iterator of the underlying vector.
   */
  auto end() const { return _items.end(); }

  /**
   * @brief Returns a future that will return the next page of items.
   *
   * @param asyncSystem The `AsyncSystem` to use.
   * @param connection The `Connection` to use to fetch the next page of
   * results.
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<T>>>
  next(CesiumAsync::AsyncSystem& asyncSystem, Connection& connection) const {
    if (!this->_nextUrl.has_value()) {
      return asyncSystem.createResolvedFuture(
          CesiumUtility::Result<PagedList<T>>(std::nullopt));
    }

    return _operation(connection, *this->_nextUrl);
  }

  /**
   * @brief Returns a future that will return the previous page of items.
   *
   * @param asyncSystem The `AsyncSystem` to use.
   * @param connection The `Connection` to use to fetch the previous page of
   * results.
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<T>>>
  prev(CesiumAsync::AsyncSystem& asyncSystem, Connection& connection) const {
    if (!this->_prevUrl.has_value()) {
      return asyncSystem.createResolvedFuture(
          CesiumUtility::Result<PagedList<T>>(std::nullopt));
    }

    return _operation(connection, *this->_prevUrl);
  }

private:
  PageOperation _operation;
  std::vector<T> _items;
  std::optional<std::string> _selfUrl = std::nullopt;
  std::optional<std::string> _nextUrl = std::nullopt;
  std::optional<std::string> _prevUrl = std::nullopt;
};
}; // namespace CesiumITwinClient