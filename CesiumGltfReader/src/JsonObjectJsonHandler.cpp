#include "JsonObjectJsonHandler.h"

using namespace CesiumGltf;

namespace {
template <typename T> void addOrReplace(JsonValue& json, T value) {
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&json.value);
  if (pArray) {
    pArray->emplace_back(value);
  } else {
    json = value;
  }
}
} // namespace

JsonObjectJsonHandler::JsonObjectJsonHandler(
    const ReadModelOptions& options) noexcept
    : JsonHandler(options) {}

void JsonObjectJsonHandler::reset(IJsonHandler* pParent, JsonValue* pValue) {
  JsonHandler::reset(pParent);
  this->_stack.clear();
  this->_stack.push_back(pValue);
}

IJsonHandler* JsonObjectJsonHandler::Null() {
  addOrReplace(*this->_stack.back(), JsonValue::Null());
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Bool(bool b) {
  addOrReplace(*this->_stack.back(), b);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Int(int i) {
  addOrReplace(*this->_stack.back(), JsonValue::Number(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Uint(unsigned i) {
  addOrReplace(*this->_stack.back(), JsonValue::Number(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Int64(int64_t i) {
  addOrReplace(*this->_stack.back(), JsonValue::Number(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Uint64(uint64_t i) {
  addOrReplace(*this->_stack.back(), JsonValue::Number(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::Double(double d) {
  addOrReplace(*this->_stack.back(), d);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::RawNumber(
    const char* /* str */,
    size_t /* length */,
    bool /* copy */) {
  return nullptr;
}

IJsonHandler* JsonObjectJsonHandler::String(
    const char* str,
    size_t /* length */,
    bool /* copy */) {
  *this->_stack.back() = str;
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::StartObject() {
  JsonValue& current = *this->_stack.back();
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&current.value);
  if (pArray) {
    JsonValue& newArray = pArray->emplace_back(JsonValue::Object());
    this->_stack.emplace_back(&newArray);
  } else {
    current = JsonValue::Object();
  }

  return this;
}

IJsonHandler* JsonObjectJsonHandler::Key(
    const char* str,
    size_t /* length */,
    bool /* copy */) {
  JsonValue& json = *this->_stack.back();
  JsonValue::Object* pObject = std::get_if<JsonValue::Object>(&json.value);

  auto it = pObject->emplace(str, JsonValue()).first;
  this->_stack.push_back(&it->second);
  this->_currentKey = str;
  return this;
}

IJsonHandler* JsonObjectJsonHandler::EndObject(size_t /* memberCount */) {
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::StartArray() {
  JsonValue& current = *this->_stack.back();
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&current.value);
  if (pArray) {
    JsonValue& newArray = pArray->emplace_back(JsonValue::Array());
    this->_stack.emplace_back(&newArray);
  } else {
    current = JsonValue::Array();
  }

  return this;
}

IJsonHandler* JsonObjectJsonHandler::EndArray(size_t /* elementCount */) {
  this->_stack.pop_back();
  return this->_stack.empty() ? this->parent() : this;
}

IJsonHandler* JsonObjectJsonHandler::doneElement() {
  JsonValue& current = *this->_stack.back();
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&current.value);
  if (!pArray) {
    this->_stack.pop_back();
    return this->_stack.empty() ? this->parent() : this;
  } else {
    return this;
  }
}