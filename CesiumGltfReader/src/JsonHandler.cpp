#include "CesiumGltf/JsonReader.h"

using namespace CesiumGltf;
JsonHandler::JsonHandler(const ReadModelOptions& options) noexcept
    : _options(options) {}

IJsonHandler* JsonHandler::Null() {
  this->reportWarning("A null value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Bool(bool /*b*/) {
  this->reportWarning("A boolean value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Int(int /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Uint(unsigned /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Int64(int64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Uint64(uint64_t /*i*/) {
  this->reportWarning("An integer value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::Double(double /*d*/) {
  this->reportWarning("A double value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler*
JsonHandler::RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
  this->reportWarning("A numeric value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler*
JsonHandler::String(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
  this->reportWarning("A string value is not allowed and has been ignored.");
  return this->parent();
}

IJsonHandler* JsonHandler::StartObject() {
  this->reportWarning("An object value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->StartObject();
}

IJsonHandler*
JsonHandler::Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) {
  return nullptr;
}

IJsonHandler* JsonHandler::EndObject(size_t /*memberCount*/) { return nullptr; }

IJsonHandler* JsonHandler::StartArray() {
  this->reportWarning("An array value is not allowed and has been ignored.");
  return this->ignoreAndReturnToParent()->StartArray();
}

IJsonHandler* JsonHandler::EndArray(size_t /*elementCount*/) { return nullptr; }

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
