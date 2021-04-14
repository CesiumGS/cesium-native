#include "CesiumJsonReader/IgnoreValueJsonHandler.h"
#include <string>

using namespace CesiumJsonReader;

void IgnoreValueJsonHandler::reset(IJsonReader* pParent) {
  this->_pParent = pParent;
  this->_depth = 0;
}

IJsonReader* IgnoreValueJsonHandler::readNull() {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readBool(bool /*b*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readInt32(int32_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readUint32(uint32_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readInt64(int64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readUint64(uint64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readDouble(double /*d*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader*
IgnoreValueJsonHandler::readString(const std::string_view& /*str*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readObjectStart() {
  ++this->_depth;
  return this;
}

IJsonReader*
IgnoreValueJsonHandler::readObjectKey(const std::string_view& /*str*/) {
  return this;
}

IJsonReader* IgnoreValueJsonHandler::readObjectEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

IJsonReader* IgnoreValueJsonHandler::readArrayStart() {
  ++this->_depth;
  return this;
}

IJsonReader* IgnoreValueJsonHandler::readArrayEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

void IgnoreValueJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  context.push_back("Ignoring a value");
  this->parent()->reportWarning(warning, std::move(context));
}

IJsonReader* IgnoreValueJsonHandler::parent() { return this->_pParent; }