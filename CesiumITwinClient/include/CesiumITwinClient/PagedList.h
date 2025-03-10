#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include <CesiumAsync/Future.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <rapidjson/document.h>

#include <optional>
#include <string>
#include <vector>
#include <functional>

namespace CesiumITwinClient {

class Connection;

template <typename T> class PagedList {
public:
  using PageOperation = std::function<CesiumAsync::Future<CesiumUtility::Result<PagedList<T>>>(Connection&, const std::string&)>;

  PagedList(rapidjson::Document& doc, std::vector<T>&& items, PageOperation&& operation) : _operation(std::move(operation)), _items(std::move(items)) {
    const auto& linksMember = doc.FindMember("_links");
    if(linksMember != doc.MemberEnd() && linksMember->value.IsObject()) {
      const auto& selfMember = linksMember->value.FindMember("self");
      if(selfMember != doc.MemberEnd() && selfMember->value.IsObject()) {
        this->_selfUrl = CesiumUtility::JsonHelpers::getStringOrDefault(selfMember->value, "href", "");
      }

      const auto& nextMember = linksMember->value.FindMember("next");
      if(nextMember != doc.MemberEnd() && nextMember->value.IsObject()) {
        this->_nextUrl = CesiumUtility::JsonHelpers::getStringOrDefault(nextMember->value, "href", "");
      }

      const auto& prevMember = linksMember->value.FindMember("prev");
      if(prevMember != doc.MemberEnd() && prevMember->value.IsObject()) {
        this->_prevUrl = CesiumUtility::JsonHelpers::getStringOrDefault(prevMember->value, "href", "");
      }
    }
  }

  T& operator[](size_t index) { return _items[index]; }

  /*CesiumAsync::Future<CesiumUtility::Result<PagedList<T>>> next(CesiumAsync::AsyncSystem& asyncSystem, Connection& connection) const {
    if(!this->_nextUrl.has_value()) {
      return asyncSystem.createResolvedFuture(CesiumUtility::Result<PagedList<T>>(std::nullopt));
    }

    return _operation(connection, *this->_nextUrl);
  }

  CesiumAsync::Future<CesiumUtility::Result<PagedList<T>>> prev(CesiumAsync::AsyncSystem& asyncSystem, Connection& connection) const {
    if(!this->_prevUrl.has_value()) {
      return asyncSystem.createResolvedFuture(CesiumUtility::Result<PagedList<T>>(std::nullopt));
    }

    return _operation(connection, *this->_prevUrl);
  }*/
private:
  PageOperation _operation;
  std::vector<T> _items;
  std::optional<std::string> _selfUrl = std::nullopt;
  std::optional<std::string> _nextUrl = std::nullopt;
  std::optional<std::string> _prevUrl = std::nullopt;
};
}; // namespace CesiumITwinClient