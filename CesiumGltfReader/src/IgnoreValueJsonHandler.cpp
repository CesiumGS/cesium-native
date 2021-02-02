#include "IgnoreValueJsonHandler.h"

using namespace CesiumGltf;

void IgnoreValueJsonHandler::reset(IJsonHandler* pParent) {
    this->_pParent = pParent;
    this->_depth = 0;
}

IJsonHandler* IgnoreValueJsonHandler::Null() {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Bool(bool /*b*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Int(int /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Uint(unsigned /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Int64(int64_t /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Uint64(uint64_t /*i*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::Double(double /*d*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::String(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::StartObject() {
    ++this->_depth; return this;
}

IJsonHandler* IgnoreValueJsonHandler::Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
    return this;
}

IJsonHandler* IgnoreValueJsonHandler::EndObject(size_t /*memberCount*/) {
    --this->_depth; return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::StartArray() {
    ++this->_depth; return this;
}

IJsonHandler* IgnoreValueJsonHandler::EndArray(size_t /*elementCount*/) {
    --this->_depth; return this->_depth == 0 ? this->parent() : this;
}

void IgnoreValueJsonHandler::reportWarning(const std::string& warning, std::vector<std::string>&& context) {
    context.push_back("Ignoring a value");
    this->parent()->reportWarning(warning, std::move(context));
}

IJsonHandler* IgnoreValueJsonHandler::parent() {
    return this->_pParent;
}