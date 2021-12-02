#include "CesiumJsonWriter/JsonWriter.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

namespace CesiumJsonWriter {
JsonWriter::JsonWriter()
    : compact(std::make_unique<rapidjson::Writer<rapidjson::StringBuffer>>(
          _compactBuffer)) {}

bool JsonWriter::Null() { return compact->Null(); }

bool JsonWriter::Bool(bool b) { return compact->Bool(b); }

bool JsonWriter::Int(int i) { return compact->Int(i); }

bool JsonWriter::Uint(unsigned int i) { return compact->Uint(i); }

bool JsonWriter::Uint64(std::uint64_t i) { return compact->Uint64(i); }

bool JsonWriter::Int64(std::int64_t i) { return compact->Int64(i); }

bool JsonWriter::Double(double d) { return compact->Double(d); }

bool JsonWriter::RawNumber(const char* str, unsigned int length, bool copy) {
  return compact->RawNumber(str, length, copy);
}

bool JsonWriter::String(std::string_view string) {
  return compact->String(
      string.data(),
      static_cast<unsigned int>(string.size()));
}

bool JsonWriter::Key(std::string_view key) {
  return compact->Key(key.data(), static_cast<unsigned int>(key.size()));
}

bool JsonWriter::StartObject() { return compact->StartObject(); }

bool JsonWriter::EndObject() { return compact->EndObject(); }

bool JsonWriter::StartArray() { return compact->StartArray(); }

bool JsonWriter::EndArray() { return compact->EndArray(); }

void JsonWriter::Primitive(std::int32_t value) { compact->Int(value); }

void JsonWriter::Primitive(std::uint32_t value) { compact->Uint(value); }

void JsonWriter::Primitive(std::int64_t value) { compact->Int64(value); }

void JsonWriter::Primitive(std::uint64_t value) { compact->Uint64(value); }

void JsonWriter::Primitive(float value) {
  compact->Double(static_cast<double>(value));
}

void JsonWriter::Primitive(double value) { compact->Double(value); }

void JsonWriter::Primitive(std::nullptr_t) { compact->Null(); }

void JsonWriter::Primitive(std::string_view string) {
  compact->String(string.data(), static_cast<unsigned int>(string.size()));
}

// Integral
void JsonWriter::KeyPrimitive(std::string_view keyName, std::int32_t value) {
  Key(keyName);
  Primitive(value);
}

void JsonWriter::KeyPrimitive(std::string_view keyName, std::uint32_t value) {
  Key(keyName);
  Primitive(value);
}

void JsonWriter::KeyPrimitive(std::string_view keyName, std::int64_t value) {
  Key(keyName);
  Primitive(value);
}

void JsonWriter::KeyPrimitive(std::string_view keyName, std::uint64_t value) {
  Key(keyName);
  Primitive(value);
}

void JsonWriter::KeyPrimitive(
    std::string_view keyName,
    std::string_view value) {
  Key(keyName);
  Primitive(value);
}

// Floating Point
void JsonWriter::KeyPrimitive(std::string_view keyName, float value) {
  Key(keyName);
  Primitive(value);
}

void JsonWriter::KeyPrimitive(std::string_view keyName, double value) {
  Key(keyName);
  Primitive(value);
}

// Null
void JsonWriter::KeyPrimitive(std::string_view keyName, std::nullptr_t value) {
  Key(keyName);
  Primitive(value);
}

// Array / Objects
void JsonWriter::KeyArray(
    std::string_view keyName,
    std::function<void(void)> insideArray) {
  Key(keyName);
  compact->StartArray();
  insideArray();
  compact->EndArray();
}

void JsonWriter::KeyObject(
    std::string_view keyName,
    std::function<void(void)> insideObject) {
  Key(keyName);
  compact->StartObject();
  insideObject();
  compact->EndObject();
}

std::string JsonWriter::toString() {
  return std::string(_compactBuffer.GetString());
}

std::string_view JsonWriter::toStringView() {
  return std::string_view(_compactBuffer.GetString(), _compactBuffer.GetSize());
}

std::vector<std::byte> JsonWriter::toBytes() {
  const auto view = this->toStringView();
  std::vector<std::byte> result(view.size(), std::byte(0));
  std::uint8_t* u8Pointer = reinterpret_cast<std::uint8_t*>(result.data());
  std::copy(view.begin(), view.end(), u8Pointer);
  return result;
}

} // namespace CesiumJsonWriter
