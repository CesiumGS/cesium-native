#pragma once

#include "ObjectJsonHandler.h"

namespace CesiumGltf {
    struct ExtensibleObject;

    class ExtensibleObjectJsonHandler : public ObjectJsonHandler {
    protected:
        void reset(IJsonHandler* pParent, ExtensibleObject* pObject);
        IJsonHandler* ExtensibleObjectKey(const char* str, ExtensibleObject& o);
    };
}
