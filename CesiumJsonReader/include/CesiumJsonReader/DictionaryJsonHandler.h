#pragma once

#include "CesiumUtility/Assert.h"
#include "IntegerJsonHandler.h"
#include "Library.h"
#include "ObjectJsonHandler.h"

#include <map>
#include <unordered_map>

namespace CesiumJsonReader {
template <typename T, typename THandler>
class CESIUMJSONREADER_API DictionaryJsonHandler : public ObjectJsonHandler {
public:
  template <typename... Ts>
  DictionaryJsonHandler(Ts&&... args) noexcept
      : ObjectJsonHandler(), _item(std::forward<Ts>(args)...) {}

  void reset(
      IJsonHandler* pParent,
      std::unordered_map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary1 = pDictionary;
  }

  void reset(IJsonHandler* pParent, std::map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary2 = pDictionary;
  }

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override {
    CESIUM_ASSERT(this->_pDictionary1 || this->_pDictionary2);

    if (this->_pDictionary1) {
      auto it = this->_pDictionary1->emplace(str, T()).first;

      return this->property(it->first.c_str(), this->_item, it->second);
    }

    auto it = this->_pDictionary2->emplace(str, T()).first;

    return this->property(it->first.c_str(), this->_item, it->second);
  }

private:
  std::unordered_map<std::string, T>* _pDictionary1 = nullptr;
  std::map<std::string, T>* _pDictionary2 = nullptr;
  THandler _item;
};
} // namespace CesiumJsonReader
