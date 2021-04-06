#pragma once

#include "CesiumGltf/JsonValue.h"
#include "CesiumGltf/Library.h"
#include <any>
#include <unordered_map>
#include <vector>

namespace CesiumGltf {
/**
 * @brief The base class for objects in a glTF that have extensions and extras.
 */
struct CESIUMGLTF_API ExtensibleObject {
  /**
   * @brief Gets an extension given its static type.
   *
   * @tparam T The type of the extension.
   * @return A pointer to the extension, or nullptr if the extension is not
   * attached to this object.
   */
  template <typename T> const T* getExtension() const noexcept {
    auto it = this->extensions.find(T::TypeName);
    if (it == this->extensions.end()) {
      return nullptr;
    }

    return std::any_cast<T>(&it->second);
  }

  /** @copydoc ExtensibleObject::getExtension */
  template <typename T> T* getExtension() noexcept {
    return const_cast<T*>(
        const_cast<const ExtensibleObject*>(this)->getExtension<T>());
  }

  JsonValue* getGenericExtension(const std::string& extensionName) noexcept;

  const JsonValue*
  getGenericExtension(const std::string& extensionName) const noexcept;

  /**
   * @brief The extensions attached to this object.
   *
   * Use {@link getExtension} to get the extension with a particular static
   * type. Use {@link getGenericExtension} to get unknowns extensions as a
   * generic {@link JsonValue}.
   */
  std::unordered_map<std::string, std::any> extensions;

  /**
   * @brief Application-specific data.
   *
   * **Implementation Note:** Although extras may have any type, it is common
   * for applications to store and access custom data as key/value pairs. As
   * best practice, extras should be an Object rather than a primitive value for
   * best portability.
   */
  JsonValue::Object extras;
};
} // namespace CesiumGltf
