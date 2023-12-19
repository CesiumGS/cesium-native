#pragma once

#include "IExtensionJsonHandler.h"
#include "JsonReaderOptions.h"
#include "ObjectJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>

#include <memory>

namespace CesiumJsonReader {

class ExtensionsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  explicit ExtensionsJsonHandler(const JsonReaderOptions& context) noexcept
      : ObjectJsonHandler(),
        _context(context),
        _pObject(nullptr),
        _currentExtensionHandler() {}

  void reset(
      IJsonHandler* pParent,
      CesiumUtility::ExtensibleObject* pObject,
      const std::string& objectType);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

private:
  const JsonReaderOptions& _context;
  CesiumUtility::ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumJsonReader
