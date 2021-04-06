#include "IgnoreValueJsonHandler.h"

using namespace CesiumGltf;

void IgnoreValueJsonHandler::reset(IJsonHandler* pParent) {
  this->_pParent = pParent;
  this->_depth = 0;
}

IJsonHandler* IgnoreValueJsonHandler::readNull() {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readBool(bool /*b*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readInt32(int /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readUint32(unsigned /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readInt64(int64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readUint64(uint64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readDouble(double /*d*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readString(
    const char* /*str*/,
    size_t /*length*/,
    bool /*copy*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readObjectStart() {
  ++this->_depth;
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::readObjectKey(
    const char* /*str*/,
    size_t /*length*/,
    bool /*copy*/) {
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::readObjectEnd(size_t /*memberCount*/) {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readArrayStart() {
  ++this->_depth;
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::readArrayEnd(size_t /*elementCount*/) {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

void IgnoreValueJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  context.push_back("Ignoring a value");
  this->parent()->reportWarning(warning, std::move(context));
}

IJsonHandler* IgnoreValueJsonHandler::parent() { return this->_pParent; }