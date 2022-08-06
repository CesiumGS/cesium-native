#pragma once

#include "IJsonHandler.h"

#include <CesiumUtility/ExtensibleObject.h>
#if defined(__GNUC__) && !defined(__clang__)
#include <experimental/any>
#else
#include <any>
#endif
#include <string_view>

namespace CesiumJsonReader {

class IExtensionJsonHandler : public IJsonHandler {
public:
  virtual void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumJsonReader
