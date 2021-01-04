#include "DoubleArrayJsonHandler.h"
#include <cassert>

using namespace CesiumGltf;

void DoubleArrayJsonHandler::reset(JsonHandler* pParent, std::vector<double>* pArray) {
    JsonHandler::reset(pParent);
    this->_pArray = pArray;
    this->_arrayIsOpen = false;
}

JsonHandler* DoubleArrayJsonHandler::StartArray() {
    if (this->_arrayIsOpen) {
        return nullptr;
    }

    this->_arrayIsOpen = true;
    return this;
}

JsonHandler* DoubleArrayJsonHandler::EndArray(size_t) {
    return this->parent();
}

JsonHandler* DoubleArrayJsonHandler::Double(double d) {
    if (!this->_arrayIsOpen) {
        return nullptr;
    }

    assert(this->_pArray);
    this->_pArray->emplace_back(d);
    return this;
}
