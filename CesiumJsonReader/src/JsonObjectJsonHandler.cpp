#include "CesiumJsonReader/JsonObjectJsonHandler.h"

#include <cstdint>

using namespace CesiumJsonReader;
using namespace CesiumUtility;

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

JsonObjectJsonHandler::JsonObjectJsonHandler() noexcept : JsonHandler() {}

void JsonObjectJsonHandler::reset(IJsonHandler* pParent, JsonValue* pValue) {
  JsonHandler::reset(pParent);
  this->_stack.clear();
  this->_stack.push_back(pValue);
}

IJsonHandler* JsonObjectJsonHandler::readNull() {
  addOrReplace(*this->_stack.back(), JsonValue::Null());
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readBool(bool b) {
  addOrReplace(*this->_stack.back(), b);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readInt32(int32_t i) {
  addOrReplace(*this->_stack.back(), std::int64_t(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readUint32(uint32_t i) {
  addOrReplace(*this->_stack.back(), std::uint64_t(i));
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readInt64(int64_t i) {
  addOrReplace(*this->_stack.back(), i);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readUint64(uint64_t i) {
  addOrReplace(*this->_stack.back(), i);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readDouble(double d) {
  addOrReplace(*this->_stack.back(), d);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readString(const std::string_view& str) {
  *this->_stack.back() = std::string(str);
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readObjectStart() {
  JsonValue& current = *this->_stack.back();
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&current.value);
  if (pArray) {
    JsonValue& newObject = pArray->emplace_back(JsonValue::Object());
    this->_stack.emplace_back(&newObject);
  } else {
    current = JsonValue::Object();
  }

  return this;
}

IJsonHandler*
JsonObjectJsonHandler::readObjectKey(const std::string_view& str) {
  JsonValue& json = *this->_stack.back();
  JsonValue::Object* pObject = std::get_if<JsonValue::Object>(&json.value);

  auto it = pObject->emplace(str, JsonValue()).first;
  this->_stack.push_back(&it->second);
  this->_currentKey = str;
  return this;
}

IJsonHandler* JsonObjectJsonHandler::readObjectEnd() {
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readArrayStart() {
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

IJsonHandler* JsonObjectJsonHandler::readArrayEnd() {
  this->_stack.pop_back();
  return this->_stack.empty() ? this->parent() : this;
}

IJsonHandler* JsonObjectJsonHandler::doneElement() {
  JsonValue& current = *this->_stack.back();
  JsonValue::Array* pArray = std::get_if<JsonValue::Array>(&current.value);
  if (!pArray) {
    this->_stack.pop_back();
    return this->_stack.empty() ? this->parent() : this;
  }
  return this;
}
