#include <CesiumJsonReader/ExtensionsJsonHandler.h>
#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonReaderOptions.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <string>
#include <string_view>

namespace CesiumJsonReader {
void ExtensionsJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumUtility::ExtensibleObject* pObject,
    const std::string& objectType) {
  ObjectJsonHandler::reset(pParent);
  this->_pObject = pObject;

  if (this->_objectType != objectType) {
    this->_objectType = objectType;
  }
}

IJsonHandler*
ExtensionsJsonHandler::readObjectKey(const std::string_view& str) {
  this->_currentExtensionHandler =
      this->_context.createExtensionHandler(str, this->_objectType);
  if (this->_currentExtensionHandler) {
    this->_currentExtensionHandler->reset(this, *this->_pObject, str);
    return &this->_currentExtensionHandler->getHandler();
  } else {
    return this->ignoreAndContinue();
  }
}
} // namespace CesiumJsonReader
