#include "AccessorJsonHandler.h"
#include "CesiumGltf/Accessor.h"
#include <cassert>
#include <string>

using namespace CesiumGltf;

void AccessorJsonHandler::reset(JsonHandler* pParent, Accessor* pAccessor) {
    ObjectJsonHandler::reset(pParent);
    this->_pAccessor = pAccessor;
}

JsonHandler* AccessorJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pAccessor);

    if ("bufferView"s == str) return property(this->_bufferView, this->_pAccessor->bufferView);
    if ("byteOffset"s == str) return property(this->_byteOffset, this->_pAccessor->byteOffset);
    if ("componentType"s == str) return property(this->_componentType, this->_pAccessor->componentType);
    if ("normalized"s == str) return property(this->_normalized, this->_pAccessor->normalized);
    if ("count"s == str) return property(this->_count, this->_pAccessor->count);
    if ("type"s == str) return property(this->_type, this->_pAccessor->type);
    if ("max"s == str) return property(this->_max, this->_pAccessor->max);
    if ("min"s == str) return property(this->_min, this->_pAccessor->min);

    return this->NamedObjectKey(str, *this->_pAccessor);
}
