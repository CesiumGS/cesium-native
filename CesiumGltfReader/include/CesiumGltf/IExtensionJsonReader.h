#pragma once

#include "CesiumGltf/IJsonReader.h"
#include <any>
#include <string_view>

namespace CesiumGltf {

class IJsonReader;
struct ExtensibleObject;

class IExtensionJsonReader : public IJsonReader {
public:
  virtual void reset(
      IJsonReader* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumGltf
