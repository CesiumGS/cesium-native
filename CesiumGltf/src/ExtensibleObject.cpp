#include "CesiumGltf/ExtensibleObject.h"

using namespace CesiumGltf;

JsonValue* ExtensibleObject::getGenericExtension(
    const std::string& extensionName) noexcept {
  return const_cast<JsonValue*>(
      const_cast<const ExtensibleObject*>(this)->getGenericExtension(
          extensionName));
}

const JsonValue* ExtensibleObject::getGenericExtension(
    const std::string& extensionName) const noexcept {
  auto it = this->extensions.find(extensionName);
  if (it == this->extensions.end()) {
    return nullptr;
  }

  const JsonValue* pValue = std::any_cast<JsonValue>(&it->second);
  return pValue;
}
