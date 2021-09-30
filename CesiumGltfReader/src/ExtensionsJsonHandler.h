#pragma once

#include "CesiumGltf/IExtensionJsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"

#include <memory>

namespace CesiumGltf {
struct ReaderContext;
struct ExtensibleObject;

class ExtensionsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  ExtensionsJsonHandler(const ReaderContext& context) noexcept
      : ObjectJsonHandler(),
        _context(context),
        _pObject(nullptr),
        _currentExtensionHandler() {}

  void reset(
      IJsonHandler* pParent,
      ExtensibleObject* pObject,
      const std::string& objectType);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

private:
  const ReaderContext& _context;
  ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumGltf
