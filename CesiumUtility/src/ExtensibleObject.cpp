#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

#include <any>
#include <string>
#include <utility>

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

  const JsonValue* pValue = std::any_cast<JsonValue>(&it->second);
  return pValue;
}
} // namespace CesiumUtility
