#pragma once

#include "CesiumJsonReader/IJsonHandler.h"
#include <any>
#include <string_view>

namespace Cesium3DTiles {

struct ExtensibleObject;

class IExtensionJsonHandler : public CesiumJsonReader::IJsonHandler {
public:
  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace Cesium3DTiles
