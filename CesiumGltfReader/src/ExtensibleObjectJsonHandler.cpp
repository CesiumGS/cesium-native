#include "ExtensibleObjectJsonHandler.h"

using namespace CesiumGltf;

void ExtensibleObjectJsonHandler::reset(IJsonHandler* pParent, ExtensibleObject* /*pObject*/) {
    ObjectJsonHandler::reset(pParent);
}

IJsonHandler* ExtensibleObjectJsonHandler::ExtensibleObjectKey(const char* /*str*/, ExtensibleObject& /*o*/) {
    // TODO: handle extensions and extras.
    return this->ignoreAndContinue();
}