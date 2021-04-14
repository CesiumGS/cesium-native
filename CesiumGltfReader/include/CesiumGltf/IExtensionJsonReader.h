#pragma once

#include "CesiumJsonReader/IJsonReader.h"
#include <any>
#include <string_view>

namespace CesiumGltf {

struct ExtensibleObject;

class IExtensionJsonReader : public CesiumJsonReader::IJsonReader {
public:
  virtual void reset(
      IJsonReader* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumGltf
