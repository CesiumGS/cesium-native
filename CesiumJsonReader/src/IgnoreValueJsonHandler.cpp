#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/IgnoreValueJsonHandler.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CesiumJsonReader {
void IgnoreValueJsonHandler::reset(IJsonHandler* pParent) noexcept {
  this->_pParent = pParent;
  this->_depth = 0;
}

IJsonHandler* IgnoreValueJsonHandler::readNull() {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readBool(bool /*b*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readInt32(int32_t /*i*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readUint32(uint32_t /*i*/) {
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

IJsonHandler*
IgnoreValueJsonHandler::readString(const std::string_view& /*str*/) {
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readObjectStart() {
  ++this->_depth;
  return this;
}

IJsonHandler*
IgnoreValueJsonHandler::readObjectKey(const std::string_view& /*str*/) {
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::readObjectEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

IJsonHandler* IgnoreValueJsonHandler::readArrayStart() {
  ++this->_depth;
  return this;
}

IJsonHandler* IgnoreValueJsonHandler::readArrayEnd() {
  --this->_depth;
  return this->_depth == 0 ? this->parent() : this;
}

void IgnoreValueJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  context.emplace_back("Ignoring a value");
  this->parent()->reportWarning(warning, std::move(context));
}

IJsonHandler* IgnoreValueJsonHandler::parent() noexcept {
  return this->_pParent;
}
} // namespace CesiumJsonReader
