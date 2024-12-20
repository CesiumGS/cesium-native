#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumJsonReader/SharedAssetJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <string>
#include <string_view>

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
