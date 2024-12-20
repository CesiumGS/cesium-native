#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonHandler.h>

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace CesiumJsonReader {
JsonHandler::JsonHandler() noexcept = default;

IJsonHandler* JsonHandler::readNull() {
  this->reportWarning("A null value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readBool(bool /*b*/) {
  this->reportWarning("A boolean value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readInt32(int32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readUint32(uint32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readInt64(int64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readUint64(uint64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readDouble(double /*d*/) {
  this->reportWarning("A double value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readString(const std::string_view& /*str*/) {
  this->reportWarning("A string value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::readObjectStart() {
  this->reportWarning("An object value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->readObjectStart();
}

IJsonHandler* JsonHandler::readObjectKey(const std::string_view& /*str*/) {
  return nullptr;
}

IJsonHandler* JsonHandler::readObjectEnd() { return nullptr; }

IJsonHandler* JsonHandler::readArrayStart() {
  this->reportWarning("An array value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->readArrayStart();
}

IJsonHandler* JsonHandler::readArrayEnd() { return nullptr; }

IJsonHandler* JsonHandler::parent() { return this->_pParent; }

IJsonHandler* JsonHandler::ignoreAndReturnToParent() {
  this->_ignore.reset(this->parent());
  return &this->_ignore;
}

IJsonHandler* JsonHandler::ignoreAndContinue() {
  this->_ignore.reset(this);
  return &this->_ignore;
}

void JsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  this->parent()->reportWarning(warning, std::move(context));
}

void JsonHandler::reset(IJsonHandler* pParent) { this->_pParent = pParent; }
} // namespace CesiumJsonReader
