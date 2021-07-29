#pragma once

#include "CesiumJsonReader/ExtensibleObject.h"
#include "CesiumJsonReader/IJsonHandler.h"
#include <any>
#include <string_view>

namespace CesiumJsonReader {

struct ExtensibleObject;

class IExtensionJsonHandler : public IJsonHandler {
public:
  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumJsonReader
