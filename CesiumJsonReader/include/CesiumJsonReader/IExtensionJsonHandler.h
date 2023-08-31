#pragma once

#include "IJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>

#include <any>
#include <string_view>

namespace CesiumJsonReader {

class IExtensionJsonHandler {
public:
  virtual ~IExtensionJsonHandler() noexcept = default;
  virtual void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
  virtual IJsonHandler& getHandler() = 0;
};

} // namespace CesiumJsonReader
