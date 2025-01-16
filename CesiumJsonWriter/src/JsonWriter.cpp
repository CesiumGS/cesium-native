#include <CesiumJsonWriter/JsonWriter.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumJsonWriter {
JsonWriter::JsonWriter()
    : _compact(std::make_unique<rapidjson::Writer<rapidjson::StringBuffer>>(
          _compactBuffer)) {}

bool JsonWriter::Null() { return _compact->Null(); }

bool JsonWriter::Bool(bool b) { return _compact->Bool(b); }

bool JsonWriter::Int(int i) { return _compact->Int(i); }

bool JsonWriter::Uint(unsigned int i) { return _compact->Uint(i); }

bool JsonWriter::Uint64(std::uint64_t i) { return _compact->Uint64(i); }

bool JsonWriter::Int64(std::int64_t i) { return _compact->Int64(i); }

bool JsonWriter::Double(double d) { return _compact->Double(d); }

bool JsonWriter::RawNumber(const char* str, unsigned int length, bool copy) {
  return _compact->RawNumber(str, length, copy);
}

bool JsonWriter::String(std::string_view string) {
  return _compact->String(
      string.data(),
      static_cast<unsigned int>(string.size()));
}

bool JsonWriter::Key(std::string_view key) {
  return _compact->Key(key.data(), static_cast<unsigned int>(key.size()));
}

bool JsonWriter::StartObject() { return _compact->StartObject(); }

bool JsonWriter::EndObject() { return _compact->EndObject(); }

bool JsonWriter::StartArray() { return _compact->StartArray(); }

bool JsonWriter::EndArray() { return _compact->EndArray(); }

void JsonWriter::Primitive(std::int32_t value) { _compact->Int(value); }

void JsonWriter::Primitive(std::uint32_t value) { _compact->Uint(value); }

void JsonWriter::Primitive(std::int64_t value) { _compact->Int64(value); }

void JsonWriter::Primitive(std::uint64_t value) { _compact->Uint64(value); }

void JsonWriter::Primitive(float value) {
  _compact->Double(static_cast<double>(value));
}

void JsonWriter::Primitive(double value) { _compact->Double(value); }

void JsonWriter::Primitive(std::nullptr_t) { _compact->Null(); }

void JsonWriter::Primitive(std::string_view string) {
  _compact->String(string.data(), static_cast<unsigned int>(string.size()));
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
    const std::function<void(void)>& insideArray) {
  Key(keyName);
  _compact->StartArray();
  insideArray();
  _compact->EndArray();
}

void JsonWriter::KeyObject(
    std::string_view keyName,
    const std::function<void(void)>& insideObject) {
  Key(keyName);
  _compact->StartObject();
  insideObject();
  _compact->EndObject();
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
