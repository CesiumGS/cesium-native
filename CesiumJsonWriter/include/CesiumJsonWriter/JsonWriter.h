#pragma once

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace CesiumJsonWriter {

template <typename Allocator = rapidjson::CrtAllocator>
class JsonWriterT {
public:
  using TypeOfCompact = rapidjson::Writer<
    rapidjson::StringBuffer,
    rapidjson::UTF8<>,
    rapidjson::UTF8<>,
    Allocator,
    rapidjson::kWriteDefaultFlags
    > ;
private:

  rapidjson::StringBuffer _compactBuffer;
  std::unique_ptr<TypeOfCompact> compact;

public:
  JsonWriterT();
  virtual ~JsonWriterT() {}

  // rapidjson methods
  virtual bool Null();
  virtual bool Bool(bool b);
  virtual bool Int(int i);
  virtual bool Uint(unsigned int i);
  virtual bool Uint64(std::uint64_t i);
  virtual bool Int64(std::int64_t i);
  virtual bool Double(double d);
  virtual bool RawNumber(const char* str, unsigned int length, bool copy);
  virtual bool Key(std::string_view string);
  virtual bool String(std::string_view string);
  virtual bool StartObject();
  virtual bool EndObject();
  virtual bool StartArray();
  virtual bool EndArray();

  // Primitive overloads
  virtual void Primitive(std::int32_t value);
  virtual void Primitive(std::uint32_t value);
  virtual void Primitive(std::int64_t value);
  virtual void Primitive(std::uint64_t value);
  virtual void Primitive(float value);
  virtual void Primitive(double value);
  virtual void Primitive(std::nullptr_t value);
  virtual void Primitive(std::string_view string);

  // Integral
  virtual void KeyPrimitive(std::string_view keyName, std::int32_t value);
  virtual void KeyPrimitive(std::string_view keyName, std::uint32_t value);
  virtual void KeyPrimitive(std::string_view keyName, std::int64_t value);
  virtual void KeyPrimitive(std::string_view keyName, std::uint64_t value);

  // String
  virtual void KeyPrimitive(std::string_view keyName, std::string_view value);

  // Floating Point
  virtual void KeyPrimitive(std::string_view keyName, float value);
  virtual void KeyPrimitive(std::string_view keyName, double value);

  // Null
  virtual void KeyPrimitive(std::string_view keyName, std::nullptr_t value);

  // Array / Objects
  virtual void
  KeyArray(std::string_view keyName, std::function<void(void)> insideArray);

  virtual void
  KeyObject(std::string_view keyName, std::function<void(void)> insideObject);

  virtual std::string toString();
  virtual std::string_view toStringView();
  virtual std::vector<std::byte> toBytes();
};
} // namespace CesiumJsonWriter

namespace CesiumJsonWriter {

template <typename T>
JsonWriterT<T>::JsonWriterT()
    : compact(std::make_unique<rapidjson::Writer<rapidjson::StringBuffer>>(
          _compactBuffer)) {}

template <typename T>
bool JsonWriterT<T>::Null() { return compact->Null(); }

template <typename T>
bool JsonWriterT<T>::Bool(bool b) { return compact->Bool(b); }

template <typename T>
bool JsonWriterT<T>::Int(int i) { return compact->Int(i); }

template <typename T>
bool JsonWriterT<T>::Uint(unsigned int i) { return compact->Uint(i); }

template <typename T>
bool JsonWriterT<T>::Uint64(std::uint64_t i) { return compact->Uint64(i); }

template <typename T>
bool JsonWriterT<T>::Int64(std::int64_t i) { return compact->Int64(i); }

template <typename T>
bool JsonWriterT<T>::Double(double d) { return compact->Double(d); }

template <typename T>
bool JsonWriterT<T>::RawNumber(const char* str, unsigned int length, bool copy) {
  return compact->RawNumber(str, length, copy);
}

template <typename T>
bool JsonWriterT<T>::String(std::string_view string) {
  return compact->String(
      string.data(),
      static_cast<unsigned int>(string.size()));
}

template <typename T>
bool JsonWriterT<T>::Key(std::string_view key) {
  return compact->Key(key.data(), static_cast<unsigned int>(key.size()));
}

template <typename T>
bool JsonWriterT<T>::StartObject() { return compact->StartObject(); }

template <typename T>
bool JsonWriterT<T>::EndObject() { return compact->EndObject(); }

template <typename T>
bool JsonWriterT<T>::StartArray() { return compact->StartArray(); }

template <typename T>
bool JsonWriterT<T>::EndArray() { return compact->EndArray(); }

template <typename T>
void JsonWriterT<T>::Primitive(std::int32_t value) { compact->Int(value); }

template <typename T>
void JsonWriterT<T>::Primitive(std::uint32_t value) { compact->Uint(value); }

template <typename T>
void JsonWriterT<T>::Primitive(std::int64_t value) { compact->Int64(value); }

template <typename T>
void JsonWriterT<T>::Primitive(std::uint64_t value) { compact->Uint64(value); }

template <typename T>
void JsonWriterT<T>::Primitive(float value) {
  compact->Double(static_cast<double>(value));
}

template <typename T>
void JsonWriterT<T>::Primitive(double value) { compact->Double(value); }

template <typename T>
void JsonWriterT<T>::Primitive(std::nullptr_t) { compact->Null(); }

template <typename T>
void JsonWriterT<T>::Primitive(std::string_view string) {
  compact->String(string.data(), static_cast<unsigned int>(string.size()));
}

// Integral
template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, std::int32_t value) {
  Key(keyName);
  Primitive(value);
}

template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, std::uint32_t value) {
  Key(keyName);
  Primitive(value);
}

template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, std::int64_t value) {
  Key(keyName);
  Primitive(value);
}

template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, std::uint64_t value) {
  Key(keyName);
  Primitive(value);
}

template <typename T>
void JsonWriterT<T>::KeyPrimitive(
    std::string_view keyName,
    std::string_view value) {
  Key(keyName);
  Primitive(value);
}

// Floating Point
template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, float value) {
  Key(keyName);
  Primitive(value);
}

template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, double value) {
  Key(keyName);
  Primitive(value);
}

// Null
template <typename T>
void JsonWriterT<T>::KeyPrimitive(std::string_view keyName, std::nullptr_t value) {
  Key(keyName);
  Primitive(value);
}

// Array / Objects
template <typename T>
void JsonWriterT<T>::KeyArray(
    std::string_view keyName,
    std::function<void(void)> insideArray) {
  Key(keyName);
  compact->StartArray();
  insideArray();
  compact->EndArray();
}

template <typename T>
void JsonWriterT<T>::KeyObject(
    std::string_view keyName,
    std::function<void(void)> insideObject) {
  Key(keyName);
  compact->StartObject();
  insideObject();
  compact->EndObject();
}


template <typename T>
std::string JsonWriterT<T>::toString() {
  return std::string(_compactBuffer.GetString());
}

template <typename T>
std::string_view JsonWriterT<T>::toStringView() {
  return std::string_view(_compactBuffer.GetString(), _compactBuffer.GetSize());
}

template <typename T>
std::vector<std::byte> JsonWriterT<T>::toBytes() {
  const auto view = this->toStringView();
  std::vector<std::byte> result(view.size(), std::byte(0));
  std::uint8_t* u8Pointer = reinterpret_cast<std::uint8_t*>(result.data());
  std::copy(view.begin(), view.end(), u8Pointer);
  return result;
}

typedef JsonWriterT<rapidjson::CrtAllocator> JsonWriter;

} // namespace CesiumJsonWriter
    