#include "CesiumGltf/GltfAccessor.h"
#include <cgltf.h>

namespace CesiumGltf {
    /*static*/ GltfAccessor GltfAccessor::createFromCollectionElement(cgltf_accessor* array, size_t arrayIndex) {
        return GltfAccessor(&array[arrayIndex]);
    }

    GltfAccessor::GltfAccessor(cgltf_accessor* p) :
        _p(p)
    {
    }
}
