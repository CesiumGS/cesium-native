#pragma once

#include "IntegerJsonHandler.h"
#include "Library.h"
#include "ObjectJsonHandler.h"

#include <map>

namespace CesiumJsonReader {
template <typename T, typename THandler>
class CESIUMJSONREADER_API DictionaryJsonHandler : public ObjectJsonHandler {
public:
  template <typename... Ts>
  DictionaryJsonHandler(Ts&&... args) noexcept
      : ObjectJsonHandler(), _item(std::forward<Ts>(args)...) {}

  void reset(IJsonHandler* pParent, std::map<std::string, T>* pDictionary) {
    ObjectJsonHandler::reset(pParent);
    this->_pDictionary = pDictionary;
  }

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override {
    assert(this->_pDictionary);

    auto it = this->_pDictionary->emplace(str, T()).first;

    return this->property(it->first.c_str(), this->_item, it->second);
  }

private:
  std::map<std::string, T>* _pDictionary = nullptr;
  THandler _item;
};
} // namespace CesiumJsonReader
