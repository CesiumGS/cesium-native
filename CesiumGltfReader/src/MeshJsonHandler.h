#pragma once

#include "CesiumGltf/Mesh.h"
#include "ObjectArrayJsonHandler.h"
#include "DoubleArrayJsonHandler.h"
#include "PrimitiveJsonHandler.h"

namespace CesiumGltf {
    struct Mesh;
    struct Primitive;

    class MeshJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Mesh* pMesh);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        Mesh* _pMesh = nullptr;
        ObjectArrayJsonHandler<Primitive, PrimitiveJsonHandler> _primitives;
        DoubleArrayJsonHandler _weights;
    };
}
