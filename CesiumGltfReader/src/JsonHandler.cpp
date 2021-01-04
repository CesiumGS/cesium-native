#include "JsonHandler.h"

using namespace CesiumGltf;

JsonHandler* JsonHandler::Null() {
    return nullptr;
}

JsonHandler* JsonHandler::Bool(bool /*b*/) {
    return nullptr;
}

JsonHandler* JsonHandler::Int(int /*i*/) {
    return nullptr;
}

JsonHandler* JsonHandler::Uint(unsigned /*i*/) {
    return nullptr;
}

JsonHandler* JsonHandler::Int64(int64_t /*i*/) {
    return nullptr;
}

JsonHandler* JsonHandler::Uint64(uint64_t /*i*/) {
    return nullptr;
}

JsonHandler* JsonHandler::Double(double /*d*/) {
    return nullptr;
}

JsonHandler* JsonHandler::RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return nullptr;
}

JsonHandler* JsonHandler::String(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return nullptr;
}

JsonHandler* JsonHandler::StartObject() { return nullptr; }
JsonHandler* JsonHandler::Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return nullptr;
}

JsonHandler* JsonHandler::EndObject(size_t /*memberCount*/) {
    return nullptr;
}

JsonHandler* JsonHandler::StartArray() {
    return nullptr;
}

JsonHandler* JsonHandler::EndArray(size_t /*elementCount*/) {
    return nullptr;
}

JsonHandler* JsonHandler::parent() {
    return this->_pParent;
}

void JsonHandler::reset(JsonHandler* pParent) {
    this->_pParent = pParent;
}
