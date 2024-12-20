#include <CesiumUtility/JsonValue.h>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

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
std::vector<std::string> CesiumUtility::JsonValue::getArrayOfStrings(
    const std::string& defaultString) const {
  if (!this->isArray())
    return std::vector<std::string>();

  const JsonValue::Array& array = this->getArray();
  std::vector<std::string> result(array.size());
  std::transform(
      array.begin(),
      array.end(),
      result.begin(),
      [&defaultString](const JsonValue& arrayValue) {
        return arrayValue.getStringOrDefault(defaultString);
      });
  return result;
}
