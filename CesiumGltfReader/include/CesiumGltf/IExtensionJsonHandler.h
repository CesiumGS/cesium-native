#pragma once

#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <any>
#include <string_view>

using namespace CesiumUtility;

namespace CesiumGltf {

class IExtensionJsonHandler : public CesiumJsonReader::IJsonHandler {
public:
  virtual void reset(
      IJsonHandler* pParentHandler,
      ExtensibleObject& o,
      const std::string_view& extensionName) = 0;
};

} // namespace CesiumGltf
