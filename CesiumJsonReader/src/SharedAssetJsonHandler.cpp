#include "CesiumJsonReader/SharedAssetJsonHandler.h"

#include "CesiumJsonReader/ExtensibleObjectJsonHandler.h"
#include "CesiumJsonReader/ExtensionsJsonHandler.h"
#include "CesiumJsonReader/JsonHandler.h"
#include "CesiumJsonReader/JsonReaderOptions.h"

namespace CesiumJsonReader {
SharedAssetJsonHandler::SharedAssetJsonHandler(
    const CesiumJsonReader::JsonReaderOptions& context) noexcept
    : ExtensibleObjectJsonHandler(context) {}

void SharedAssetJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumUtility::ExtensibleObject* pObject) {
  ExtensibleObjectJsonHandler::reset(pParent, pObject);
}

CesiumJsonReader::IJsonHandler*
SharedAssetJsonHandler::readObjectKeySharedAsset(
    const std::string& objectType,
    const std::string_view& str,
    CesiumUtility::ExtensibleObject& o) {
  return this->readObjectKeyExtensibleObject(objectType, str, o);
}
} // namespace CesiumJsonReader
