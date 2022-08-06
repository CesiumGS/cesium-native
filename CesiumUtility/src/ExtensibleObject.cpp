#include "CesiumUtility/ExtensibleObject.h"

namespace CesiumUtility {
JsonValue* ExtensibleObject::getGenericExtension(
    const std::string& extensionName) noexcept {
  return const_cast<JsonValue*>(
      std::as_const(*this).getGenericExtension(extensionName));
}

const JsonValue* ExtensibleObject::getGenericExtension(
    const std::string& extensionName) const noexcept {
  auto it = this->extensions.find(extensionName);
  if (it == this->extensions.end()) {
    return nullptr;
  }

#if defined(__GNUC__) && !defined(__clang__)
  const JsonValue* pValue = std::experimental::any_cast<JsonValue>(&it->second);
#else
  const JsonValue* pValue = std::any_cast<JsonValue>(&it->second);
#endif
  return pValue;
}
} // namespace CesiumUtility
