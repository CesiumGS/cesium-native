#include "CesiumJsonWriter/IgnoreValueJsonHandler.h"

#include <string>

using namespace CesiumJsonWriter;

void IgnoreValueJsonHandler::reset(IJsonHandler* pParent) noexcept {
  this->_pParent = pParent;
  this->_depth = 0;
}

IJsonHandler* IgnoreValueJsonHandler::writeNull() {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeBool(bool /*b*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeInt32(int32_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeUint32(uint32_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeInt64(int64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeUint64(uint64_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeDouble(double /*d*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler*
IgnoreValueJsonHandler::writeString(const std::string_view& /*str*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeObjectStart() {
  ++this->_depth;
  return this;
}

IJsonHandler*
IgnoreValueJsonHandler::writeObjectKey(const std::string_view& /*str*/) {
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::writeObjectEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::writeArrayStart() {
  ++this->_depth;
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::writeArrayEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

void IgnoreValueJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  context.push_back("Ignoring a value");
  this->parent()->reportWarning(warning, std::move(context));
}

IJsonHandler* IgnoreValueJsonHandler::parent() noexcept {
  return this->_pParent;
}
