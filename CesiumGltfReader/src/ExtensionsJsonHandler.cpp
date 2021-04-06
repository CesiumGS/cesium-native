#include "ExtensionsJsonHandler.h"

using namespace CesiumGltf;

void ExtensionsJsonHandler::reset(
    IJsonHandler* pParent,
    ExtensibleObject* pObject,
    const std::string& objectType) {
  ObjectJsonHandler::reset(pParent);
  this->_pObject = pObject;

  if (this->_objectType != objectType) {
    this->_objectType = objectType;
  }
}

IJsonHandler* ExtensionsJsonHandler::readObjectKey(
    const char* str,
    size_t /* length */,
    bool /* copy */) {
  Extension* pExtension = this->_options.pExtensions->findExtension(str);
  if (pExtension) {
    this->_currentExtensionHandler = pExtension->readExtension(
        this->_options,
        str,
        *this->_pObject,
        this,
        this->_objectType);
    return this->_currentExtensionHandler.get();
  } else {
    return this->ignoreAndContinue();
  }
}
