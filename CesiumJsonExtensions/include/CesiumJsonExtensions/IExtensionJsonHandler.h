#pragma once

#include "CesiumJsonExtensions/ExtensibleObject.h"
#include "CesiumJsonReader/IJsonHandler.h"
#include <any>
#include <string_view>

namespace CesiumJsonExtensions {

struct ExtensibleObject;

class IExtensionJsonHandler : public CesiumJsonReader::IJsonHandler {
public:
  virtual void reset(
      CesiumJsonReader::IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumJsonExtensions
