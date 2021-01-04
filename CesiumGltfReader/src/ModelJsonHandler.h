#pragma once

#include "ObjectJsonHandler.h"
#include "ObjectArrayJsonHandler.h"
#include "CesiumGltf/Model.h"
#include "AccessorJsonHandler.h"
#include "MeshJsonHandler.h"

namespace CesiumGltf {
    class ModelJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Model* pModel);
        virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

    private:
        Model* _pModel;
        ObjectArrayJsonHandler<Accessor, AccessorJsonHandler> _accessors;
        ObjectArrayJsonHandler<Mesh, MeshJsonHandler> _meshes;
    };
}
