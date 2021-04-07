#pragma once

#include "ObjectJsonHandler.h"

namespace CesiumGltf {

class ExtensionsJsonHandler : public ObjectJsonHandler {
public:
  ExtensionsJsonHandler(const ReadModelOptions& options) noexcept
      : ObjectJsonHandler(options),
        _pObject(nullptr),
        _handlers(),
        _currentExtensionHandler() {}

  void reset(
      IJsonHandler* pParent,
      ExtensibleObject* pObject,
      const std::string& objectType);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

private:
  ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unordered_map<Extension*, std::unique_ptr<JsonHandler>> _handlers;
  std::unique_ptr<IJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumGltf
