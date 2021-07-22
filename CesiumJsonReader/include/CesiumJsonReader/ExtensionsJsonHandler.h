#pragma once

#include "CesiumJsonReader/IExtensionJsonHandler.h"
#include "CesiumJsonReader/ObjectJsonHandler.h"
#include <memory>

namespace CesiumJsonReader {
struct ExtensionContext;
struct ExtensibleObject;

class ExtensionsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  explicit ExtensionsJsonHandler(const ExtensionContext& context) noexcept
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
  const ExtensionContext& _context;
  ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumJsonReader
