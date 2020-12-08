#include "CesiumGltf/GltfMesh.h"
#include <cgltf.h>

namespace CesiumGltf {
    /*static*/ GltfMesh GltfMesh::createFromCollectionElement(cgltf_mesh* array, size_t arrayIndex) {
        return GltfMesh(&array[arrayIndex]);
    }

    GltfMesh::GltfMesh(cgltf_mesh* p) :
        _p(p)
    {
    }
}
