#pragma once

#include "ObjectJsonHandler.h"

namespace CesiumGltf {

class ExtensionsJsonHandler : public ObjectJsonHandler {
public:
  ExtensionsJsonHandler(const JsonReaderContext& context) noexcept
      : ObjectJsonHandler(context),
        _pObject(nullptr),
        _currentExtensionHandler() {}

  void reset(
      IJsonHandler* pParent,
      ExtensibleObject* pObject,
      const std::string& objectType);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

private:
  ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonReader> _currentExtensionHandler;
};

} // namespace CesiumGltf
