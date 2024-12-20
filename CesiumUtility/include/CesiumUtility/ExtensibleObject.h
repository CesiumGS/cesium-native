#pragma once

#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Library.h>

#include <any>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CesiumUtility {
/**
 * @brief The base class for objects that have extensions and extras.
 */
struct CESIUMUTILITY_API ExtensibleObject {
  /**
   * @brief Checks if an extension exists given its static type.
   *
   * @tparam T The type of the extension.
   * @return A boolean indicating whether the extension exists.
   */
  template <typename T> bool hasExtension() const noexcept {
    auto it = this->extensions.find(T::ExtensionName);
    return it != this->extensions.end();
  }

  /**
   * @brief Gets an extension given its static type.
   *
   * @tparam T The type of the extension.
   * @return A pointer to the extension, or nullptr if the extension is not
   * attached to this object.
   */
  template <typename T> const T* getExtension() const noexcept {
    auto it = this->extensions.find(T::ExtensionName);
    if (it == this->extensions.end()) {
      return nullptr;
    }

    return std::any_cast<T>(&it->second);
  }

  /** @copydoc ExtensibleObject::getExtension */
  template <typename T> T* getExtension() noexcept {
    return const_cast<T*>(std::as_const(*this).getExtension<T>());
  }

  /**
   * @brief Gets a generic extension with the given name as a
   * {@link CesiumUtility::JsonValue}.
   *
   * If the extension exists but has a static type, this method will return
   * nullptr. Use {@link getExtension} to retrieve a statically-typed extension.
   *
   * @param extensionName The name of the extension.
   * @return The generic extension, or nullptr if the generic extension doesn't
   * exist.
   */
  const JsonValue*
  getGenericExtension(const std::string& extensionName) const noexcept;

  /** @copydoc ExtensibleObject::getGenericExtension */
  JsonValue* getGenericExtension(const std::string& extensionName) noexcept;

  /**
   * @brief Adds a statically-typed extension to this object.
   *
   * If the extension already exists, the existing one is returned.
   *
   * @tparam T The type of the extension to add.
   * @return The added extension.
   */
  template <typename T, typename... ConstructorArgumentTypes>
  T& addExtension(ConstructorArgumentTypes&&... constructorArguments) {
    std::any& extension =
        extensions
            .try_emplace(
                T::ExtensionName,
                std::make_any<T>(std::forward<ConstructorArgumentTypes>(
                    constructorArguments)...))
            .first->second;
    return std::any_cast<T&>(extension);
  }

  /**
   * @brief Removes a statically-typed extension from this object.
   *
   * @tparam T The type of the extension to remove.
   */
  template <typename T> void removeExtension() {
    extensions.erase(T::ExtensionName);
  }

  /**
   * @brief The extensions attached to this object.
   *
   * Use {@link getExtension} to get the extension with a particular static
   * type. Use {@link getGenericExtension} to get unknown extensions as a
   * generic {@link CesiumUtility::JsonValue}.
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

  /**
   * @brief Unknown properties that exist on this object but do not have any
   * representation in the statically-typed classes.
   *
   * These properties may be invalid, or they may represent deprecated,
   * experimental, or next-version properties.
   */
  JsonValue::Object unknownProperties;

  /**
   * @brief Calculates the size in bytes of this ExtensibleObject, including all
   * of its extras but NOT including its extensions. Calling this method may be
   * slow as it requires traversing the entire object.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(ExtensibleObject));

    accum += int64_t(
        this->extras.size() * (sizeof(std::string) + sizeof(JsonValue)));
    for (const auto& [k, v] : this->extras) {
      accum += int64_t(k.capacity() * sizeof(char) - sizeof(std::string));
      accum += v.getSizeBytes() - int64_t(sizeof(JsonValue));
    }

    accum += int64_t(
        this->unknownProperties.size() *
        (sizeof(std::string) + sizeof(JsonValue)));
    for (const auto& [k, v] : this->unknownProperties) {
      accum += int64_t(k.capacity() * sizeof(char) - sizeof(std::string));
      accum += v.getSizeBytes() - int64_t(sizeof(JsonValue));
    }

    return accum;
  }
};
} // namespace CesiumUtility
