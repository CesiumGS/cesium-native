#pragma once

#include "CesiumGltf/IJsonReader.h"
#include <any>
#include <string_view>

namespace CesiumGltf {

class IJsonHandler;
struct ExtensibleObject;

class IExtensionJsonReader : public IJsonHandler {
public:
  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumGltf
