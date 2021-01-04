#include "AlphaModeJsonHandler.h"
#include <string>

using namespace CesiumGltf;

void AlphaModeJsonHandler::reset(JsonHandler* pParent, AlphaMode* pEnum) {
    JsonHandler::reset(pParent);
    this->_pEnum = pEnum;
}

JsonHandler* AlphaModeJsonHandler::String(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    if ("OPAQUE"s == str) *this->_pEnum = AlphaMode::OPAQUE;
    else if ("MASK"s == str) *this->_pEnum = AlphaMode::MASK;
    else if ("BLEND"s == str) *this->_pEnum = AlphaMode::BLEND;
    else return nullptr;

    return this->parent();
}
