#include "PrimitiveJsonHandler.h"
#include "CesiumGltf/Primitive.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void PrimitiveJsonHandler::reset(JsonHandler* pParent, Primitive* pPrimitive) {
    JsonHandler::reset(pParent);
    this->_pPrimitive = pPrimitive;
}

JsonHandler* PrimitiveJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pPrimitive);
    if ("indices"s == str) return property(this->_indices, this->_pPrimitive->indices);
    if ("material"s == str) return property(this->_material, this->_pPrimitive->material);
    if ("mode"s == str) return property(this->_mode, this->_pPrimitive->mode);

    return this->ignore();
}
