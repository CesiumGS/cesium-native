#pragma once

#include "IntegerJsonHandler.h"
#include "ObjectJsonHandler.h"
#include <CesiumGltf/Reader.h>
#include <map>
#include <unordered_map>

namespace CesiumGltf {
template <typename T, typename THandler>
class DictionaryJsonHandler : public ObjectJsonHandler {
public:
  DictionaryJsonHandler(const ReaderContext& context) noexcept
      : ObjectJsonHandler(context), _item(context) {}

  void
  reset(IJsonReader* pParent, std::unordered_map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary1 = pDictionary;
  }

  void reset(IJsonReader* pParent, std::map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary2 = pDictionary;
  }

  virtual IJsonReader* readObjectKey(const std::string_view& str) override {
    assert(this->_pDictionary1 || this->_pDictionary2);

    if (this->_pDictionary1) {
      auto it = this->_pDictionary1->emplace(str, T()).first;

      return this->property(it->first.c_str(), this->_item, it->second);
    } else {
      auto it = this->_pDictionary2->emplace(str, T()).first;

      return this->property(it->first.c_str(), this->_item, it->second);
    }
  }

private:
  std::unordered_map<std::string, T>* _pDictionary1 = nullptr;
  std::map<std::string, T>* _pDictionary2 = nullptr;
  THandler _item;
};
} // namespace CesiumGltf
