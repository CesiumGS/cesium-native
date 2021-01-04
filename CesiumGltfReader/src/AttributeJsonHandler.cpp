#include "AttributeJsonHandler.h"

using namespace CesiumGltf;

void AttributeJsonHandler::reset(JsonHandler* pParent, std::unordered_map<std::string, int32_t>* pAttributes) {
    ObjectJsonHandler::reset(pParent);
    this->_pAttributes = pAttributes;
}

JsonHandler* AttributeJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    assert(this->_pAttributes);

    return this->property(
        this->_index,
        this->_pAttributes->emplace(str, -1).first->second
    );
}
