#include "CesiumGltf/GltfAccessor.h"
#include <tiny_gltf.h>

namespace CesiumGltf {
    GltfAccessor::GltfAccessor() :
        _p(new tinygltf_type()),
        _owns(true)
    {
    }

    GltfAccessor::~GltfAccessor() {
        if (this->_owns) {
            delete this->_p;
        }
    }

    // /*static*/ GltfAccessor GltfAccessor::createFromCollectionElement(cgltf_accessor* array, size_t arrayIndex) {
    //     return GltfAccessor(&array[arrayIndex]);
    // }

    // GltfAccessor::GltfAccessor(cgltf_accessor* p) :
    //     _p(p)
    // {
    // }

    GltfAccessor::GltfAccessor(tinygltf::Accessor* p) :
        _p(p),
        _owns(false)
    {
    }
}
