#include "CesiumGltf/GltfAccessor.h"
#include <tiny_gltf.h>

#define CESIUM_GLTF_WRAPPER_IMPL(Type) \
    Type##Const::Type##Const() : \
        _p(new Tiny()), \
        _owns(true) \
    {} \
    Type##Const::Type##Const(const Type##Const& rhs) : \
        _p(new Tiny(*rhs._p)), \
        _owns(true) \
    {} \
    Type##Const::~Type##Const() { \
        if (this->_owns) { \
            delete this->_p; \
        } \
    } \
    Type##Const::Type##Const(const Tiny* p) : \
        _p(p), \
        _owns(false) \
    {} \
    Type::Type() : Type##Const() {} \
    Type::Type(const Type& rhs) : Type##Const(rhs) {} \
    Type::Type(GltfAccessor&& rhs) : \
        Type(static_cast<Type##Const&&>(std::move(rhs))) \
    {} \
    Type::Type(const Type##Const& rhs) : \
        Type##Const(rhs) \
    {} \
    Type::Type(Type##Const&& rhs) : \
        Type##Const(std::exchange(rhs._p, nullptr)) \
    { \
        this->_owns = rhs._owns; \
    } \
    Type& Type::operator=(const Type& rhs) { \
        *this->tiny() = *rhs._p; \
        return *this; \
    } \
    Type& Type::operator=(Type&& rhs) { \
        std::swap(this->_p, rhs._p); \
        std::swap(this->_owns, rhs._owns); \
        return *this; \
    } \
    Type& Type::operator=(const Type##Const& rhs) { \
        *this->tiny() = *rhs._p; \
        return *this; \
    }

namespace CesiumGltf {
    CESIUM_GLTF_WRAPPER_IMPL(GltfAccessor);

    size_t GltfAccessorConst::getCount() const {
        return this->_p->count;
    }

    void GltfAccessor::setCount(size_t value) {
        this->tiny()->count = value;
    }
}
