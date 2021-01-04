#pragma once

#include "CesiumGltf/Mesh.h"
#include "DoubleArrayJsonHandler.h"
#include "NamedObjectJsonHandler.h"
#include "ObjectArrayJsonHandler.h"
#include "PrimitiveJsonHandler.h"

namespace CesiumGltf {
    struct Mesh;
    struct Primitive;

    class MeshJsonHandler : public NamedObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Mesh* pMesh);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        Mesh* _pMesh = nullptr;
        ObjectArrayJsonHandler<Primitive, PrimitiveJsonHandler> _primitives;
        DoubleArrayJsonHandler _weights;
    };
}
