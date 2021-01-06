#pragma once

#include "ObjectJsonHandler.h"

namespace CesiumGltf {
    struct ExtensibleObject;

    class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
    protected:
        IJsonHandler* ExtensibleObjectKey(const char* str, ExtensibleObject& o);
    };
}
