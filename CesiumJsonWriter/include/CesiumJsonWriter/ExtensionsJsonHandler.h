#pragma once

#include "ExtensionWriterContext.h"
#include "IExtensionJsonHandler.h"
#include "ObjectJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>

#include <memory>

namespace CesiumJsonWriter {

class ExtensionsJsonHandler : public CesiumJsonWriter::ObjectJsonHandler {
public:
  explicit ExtensionsJsonHandler(const ExtensionWriterContext& context) noexcept
      : ObjectJsonHandler(),
        _context(context),
        _pObject(nullptr),
        _currentExtensionHandler() {}

  void reset(
      IJsonHandler* pParent,
      CesiumUtility::ExtensibleObject* pObject,
      const std::string& objectType);

  virtual IJsonHandler* writeObjectKey(const std::string_view& str) override;

private:
  const ExtensionWriterContext& _context;
  CesiumUtility::ExtensibleObject* _pObject = nullptr;
  std::string _objectType;
  std::unique_ptr<IExtensionJsonHandler> _currentExtensionHandler;
};

} // namespace CesiumJsonWriter
