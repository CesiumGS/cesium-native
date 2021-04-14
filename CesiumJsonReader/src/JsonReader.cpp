#include "CesiumJsonReader/JsonReader.h"

using namespace CesiumJsonReader;

JsonReader::JsonReader() noexcept {}

IJsonReader* JsonReader::readNull() {
  this->reportWarning("A null value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readBool(bool /*b*/) {
  this->reportWarning("A boolean value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readInt32(int32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readUint32(uint32_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readInt64(int64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readUint64(uint64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readDouble(double /*d*/) {
  this->reportWarning("A double value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readString(const std::string_view& /*str*/) {
  this->reportWarning("A string value is not allowed and has been ignored.");
  return this->parent();
}

IJsonReader* JsonReader::readObjectStart() {
  this->reportWarning("An object value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->readObjectStart();
}

IJsonReader* JsonReader::readObjectKey(const std::string_view& /*str*/) {
  return nullptr;
}

IJsonReader* JsonReader::readObjectEnd() { return nullptr; }

IJsonReader* JsonReader::readArrayStart() {
  this->reportWarning("An array value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->readArrayStart();
}

IJsonReader* JsonReader::readArrayEnd() { return nullptr; }

IJsonReader* JsonReader::parent() { return this->_pParent; }

IJsonReader* JsonReader::ignoreAndReturnToParent() {
  this->_ignore.reset(this->parent());
  return &this->_ignore;
}

IJsonReader* JsonReader::ignoreAndContinue() {
  this->_ignore.reset(this);
  return &this->_ignore;
}

void JsonReader::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  this->parent()->reportWarning(warning, std::move(context));
}

void JsonReader::reset(IJsonReader* pParent) { this->_pParent = pParent; }
