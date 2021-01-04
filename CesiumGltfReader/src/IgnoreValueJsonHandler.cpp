#include "IgnoreValueJsonHandler.h"

using namespace CesiumGltf;

void IgnoreValueJsonHandler::reset(JsonHandler* pParent) {
    JsonHandler::reset(pParent);
    this->_depth = 0;
}

CesiumGltf::JsonHandler* IgnoreValueJsonHandler::Null() {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Bool(bool /*b*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Int(int /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Uint(unsigned /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Int64(int64_t /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Uint64(uint64_t /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::Double(double /*d*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::String(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::StartObject() {
    ++this->_depth; return this;
}

JsonHandler* IgnoreValueJsonHandler::Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this;
}

JsonHandler* IgnoreValueJsonHandler::EndObject(size_t /*memberCount*/) {
    --this->_depth; return this->_depth == 0 ? this->parent() : this;
}

JsonHandler* IgnoreValueJsonHandler::StartArray() {
    ++this->_depth; return this;
}

JsonHandler* IgnoreValueJsonHandler::EndArray(size_t /*elementCount*/) {
    --this->_depth; return this->_depth == 0 ? this->parent() : this;
}
