#pragma once

#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

#include <string_view>

namespace CesiumGltfReader {

class SharedAssetJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  explicit SharedAssetJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& context) noexcept;

protected:
  void reset(IJsonHandler* pParent, CesiumUtility::ExtensibleObject* pObject);
  IJsonHandler* readObjectKeySharedAsset(
      const std::string& objectType,
      const std::string_view& str,
      CesiumUtility::ExtensibleObject& o);

private:
};
} // namespace CesiumGltfReader
