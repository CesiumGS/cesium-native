#include "DoubleJsonHandler.h"

using namespace CesiumGltf;

void DoubleJsonHandler::reset(JsonHandler* pParent, double* pDouble) {
    JsonHandler::reset(pParent);
    this->_pDouble = pDouble;
}

JsonHandler* DoubleJsonHandler::Int(int i) {
    assert(this->_pDouble);
    *this->_pDouble = static_cast<double>(i);
    return this->parent();
}

JsonHandler* DoubleJsonHandler::Uint(unsigned i) {
    assert(this->_pDouble);
    *this->_pDouble = static_cast<double>(i);
    return this->parent();
}

JsonHandler* DoubleJsonHandler::Int64(int64_t i) {
    assert(this->_pDouble);
    *this->_pDouble = static_cast<double>(i);
    return this->parent();
}

JsonHandler* DoubleJsonHandler::Uint64(uint64_t i) {
    assert(this->_pDouble);
    *this->_pDouble = static_cast<double>(i);
    return this->parent();
}

JsonHandler* DoubleJsonHandler::Double(double d) {
    assert(this->_pDouble);
    *this->_pDouble = d;
    return this->parent();
}
