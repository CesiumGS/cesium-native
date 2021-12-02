#include "CesiumUtility/JsonValue.h"

namespace CesiumUtility {

const JsonValue* JsonValue::getValuePtrForKey(const std::string& key) const {
  const Object* pObject = std::get_if<Object>(&this->value);
  if (!pObject) {
    return nullptr;
  }

  auto it = pObject->find(key);
  if (it == pObject->end()) {
    return nullptr;
  }

  return &it->second;
}

JsonValue* JsonValue::getValuePtrForKey(const std::string& key) {
  Object* pObject = std::get_if<Object>(&this->value);
  if (!pObject) {
    return nullptr;
  }

  auto it = pObject->find(key);
  if (it == pObject->end()) {
    return nullptr;
  }

  return &it->second;
}

} // namespace CesiumUtility
