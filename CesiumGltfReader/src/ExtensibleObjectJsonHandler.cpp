#include "ExtensibleObjectJsonHandler.h"

using namespace CesiumGltf;

JsonHandler* ExtensibleObjectJsonHandler::ExtensibleObjectKey(const char* /*str*/, ExtensibleObject& /*o*/) {
    // TODO: handle extensions and extras.
    return this->ignore();
}