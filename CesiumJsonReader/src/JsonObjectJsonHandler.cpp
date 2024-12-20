#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumUtility/JsonValue.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace CesiumJsonReader {
namespace {
template <typename T>
void addOrReplace(CesiumUtility::JsonValue& json, T value) {
  CesiumUtility::JsonValue::Array* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&json.value);
  if (pArray) {
    pArray->emplace_back(value);
  } else {
    json = value;
  }
}
} // namespace

JsonObjectJsonHandler::JsonObjectJsonHandler() noexcept : JsonHandler() {}

void JsonObjectJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumUtility::JsonValue* pValue) {
  JsonHandler::reset(pParent);
  this->_stack.clear();
  this->_stack.push_back(pValue);
}

IJsonHandler* JsonObjectJsonHandler::readNull() {
  addOrReplace(*this->_stack.back(), CesiumUtility::JsonValue::Null());
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
  CesiumUtility::JsonValue& current = *this->_stack.back();
  CesiumUtility::JsonValue::Array* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&current.value);
  if (pArray) {
    pArray->emplace_back(std::string(str));
  } else {
    current = std::string(str);
  }

  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readObjectStart() {
  CesiumUtility::JsonValue& current = *this->_stack.back();
  CesiumUtility::JsonValue::Array* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&current.value);
  if (pArray) {
    CesiumUtility::JsonValue& newObject =
        pArray->emplace_back(CesiumUtility::JsonValue::Object());
    this->_stack.emplace_back(&newObject);
  } else {
    current = CesiumUtility::JsonValue::Object();
  }

  return this;
}

IJsonHandler*
JsonObjectJsonHandler::readObjectKey(const std::string_view& str) {
  CesiumUtility::JsonValue& json = *this->_stack.back();
  CesiumUtility::JsonValue::Object* pObject =
      std::get_if<CesiumUtility::JsonValue::Object>(&json.value);

  auto it = pObject->emplace(str, CesiumUtility::JsonValue()).first;
  this->_stack.push_back(&it->second);
  this->_currentKey = str;
  return this;
}

IJsonHandler* JsonObjectJsonHandler::readObjectEnd() {
  return this->doneElement();
}

IJsonHandler* JsonObjectJsonHandler::readArrayStart() {
  CesiumUtility::JsonValue& current = *this->_stack.back();
  CesiumUtility::JsonValue::Array* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&current.value);
  if (pArray) {
    CesiumUtility::JsonValue& newArray =
        pArray->emplace_back(CesiumUtility::JsonValue::Array());
    this->_stack.emplace_back(&newArray);
  } else {
    current = CesiumUtility::JsonValue::Array();
  }

  return this;
}

IJsonHandler* JsonObjectJsonHandler::readArrayEnd() {
  this->_stack.pop_back();
  return this->_stack.empty() ? this->parent() : this;
}

IJsonHandler* JsonObjectJsonHandler::doneElement() {
  CesiumUtility::JsonValue& current = *this->_stack.back();
  CesiumUtility::JsonValue::Array* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&current.value);
  if (!pArray) {
    this->_stack.pop_back();
    return this->_stack.empty() ? this->parent() : this;
  }
  return this;
}
} // namespace CesiumJsonReader
