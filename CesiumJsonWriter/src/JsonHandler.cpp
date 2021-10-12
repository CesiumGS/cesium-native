#include "CesiumJsonWriter/JsonHandler.h"

using namespace CesiumJsonWriter;

JsonHandler::JsonHandler() noexcept {}

IJsonHandler* JsonHandler::writeNull() {
  this->reportWarning("A null value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeBool(bool /*b*/) {
  this->reportWarning("A boolean value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeInt32(int32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeUint32(uint32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeInt64(int64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeUint64(uint64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeDouble(double /*d*/) {
  this->reportWarning("A double value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeString(const std::string_view& /*str*/) {
  this->reportWarning("A string value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::writeObjectStart() {
  this->reportWarning("An object value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->writeObjectStart();
}

IJsonHandler* JsonHandler::writeObjectKey(const std::string_view& /*str*/) {
  return nullptr;
}

IJsonHandler* JsonHandler::writeObjectEnd() { return nullptr; }

IJsonHandler* JsonHandler::writeArrayStart() {
  this->reportWarning("An array value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->writeArrayStart();
}

IJsonHandler* JsonHandler::writeArrayEnd() { return nullptr; }

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
