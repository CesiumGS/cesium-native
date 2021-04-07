#pragma once

#include "CesiumGltf/Extension.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace CesiumGltf {

class ExtensionRegistry {
public:
  static std::shared_ptr<ExtensionRegistry> getDefault();

  template <typename T> void registerExtension() {
    this->_extensions.emplace(T::ExtensionName, std::make_shared<T>());
  }

  template <typename T> void registerDefault() {
    this->_pDefault = std::make_shared<T>();
  }

  void clearDefault() { this->_pDefault.reset(); }

  Extension* findExtension(const std::string_view& name) const;

private:
  std::unordered_map<std::string, std::shared_ptr<Extension>> _extensions;
  std::shared_ptr<Extension> _pDefault;
};

} // namespace CesiumGltf
