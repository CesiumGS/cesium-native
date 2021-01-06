#include "ExtensibleObjectJsonHandler.h"

using namespace CesiumGltf;

IJsonHandler* ExtensibleObjectJsonHandler::ExtensibleObjectKey(const char* /*str*/, ExtensibleObject& /*o*/) {
    // TODO: handle extensions and extras.
    return this->ignoreAndContinue();
}