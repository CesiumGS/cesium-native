#pragma once

#include "JsonWriter.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace CesiumJsonWriter {

template <typename Allocator = rapidjson::CrtAllocator>
class PrettyJsonWriter : public JsonWriter {
  rapidjson::StringBuffer _prettyBuffer;

public:
  using TypeOfPretty = rapidjson::PrettyWriter<
    rapidjson::StringBuffer,
    rapidjson::UTF8<>,
    rapidjson::UTF8<>,
    Allocator,
    rapidjson::kWriteDefaultFlags
    > ;

private:
  std::unique_ptr<TypeOfPretty> pretty;

public:
  PrettyJsonWriter() noexcept {
  auto writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>(_prettyBuffer);
  writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  pretty = std::make_unique<TypeOfPretty>(
      std::move(writer));
  pretty->SetIndent(' ', 2);
}

  ~PrettyJsonWriter() {}

  // rapidjson methods
  bool Null() override  { return pretty->Null(); }
  bool Bool(bool b) override { return pretty->Bool(b); }
  bool Int(int i) override  { return pretty->Int(i); }
  bool Uint(unsigned int i) override  { return pretty->Uint(i); }
  bool Uint64(std::uint64_t i) override  { return pretty->Uint64(i); }
  bool Int64(std::int64_t i) override  { return pretty->Int64(i); }
  bool Double(double d) override { return pretty->Double(d); }
  bool RawNumber(const char* str, unsigned int length, bool copy) override {
  return pretty->RawNumber(str, length, copy);
}
  bool Key(std::string_view key) override {
  return pretty->Key(key.data(), static_cast<unsigned int>(key.size()));
}

  bool String(std::string_view string) override  {
  return pretty->String(
      string.data(),
      static_cast<unsigned int>(string.size()));
}
  bool StartObject() override  { return pretty->StartObject(); }
  bool EndObject() override  { return pretty->EndObject(); }
  bool StartArray() override { return pretty->StartArray(); }
  bool EndArray() override  { return pretty->EndArray(); }

  // Primitive overloads
  void Primitive(std::int32_t value) override  { pretty->Int(value); }
  void Primitive(std::uint32_t value) override  { pretty->Uint(value); }
  void Primitive(std::int64_t value) override  { pretty->Int64(value); }
  void Primitive(std::uint64_t value) override  { pretty->Uint64(value); }
  void Primitive(float value) override {
  pretty->Double(static_cast<double>(value));
}
  void Primitive(double value) override  { pretty->Double(value); }
  void Primitive(std::nullptr_t value) override  { pretty->Null(); }
  void Primitive(std::string_view string) override  {
  pretty->String(string.data(), static_cast<unsigned int>(string.size()));
}

  void KeyPrimitive(std::string_view keyName, std::int32_t value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, std::uint32_t value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, std::int64_t value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, std::uint64_t value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, float value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, double value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, std::nullptr_t value) override {
  Key(keyName);
  Primitive(value);
}

  void KeyPrimitive(std::string_view keyName, std::string_view value) override {
  Key(keyName);
  Primitive(value);
}

  // Array / Objects
  void KeyArray(std::string_view keyName, std::function<void(void)> insideArray)
      override {
  Key(keyName);
  pretty->StartArray();
  insideArray();
  pretty->EndArray();
}

  void KeyObject(
      std::string_view keyName,
      std::function<void(void)> insideObject) override  {
  Key(keyName);
  pretty->StartObject();
  insideObject();
  pretty->EndObject();
}

  std::string toString() override  {
  return std::string(_prettyBuffer.GetString());
}
  std::string_view toStringView() override  {
  return std::string_view(_prettyBuffer.GetString());
}
  std::vector<std::byte> toBytes() override  {
  const auto view = this->toStringView();
  std::vector<std::byte> result(view.size(), std::byte(0));
  std::uint8_t* u8Pointer = reinterpret_cast<std::uint8_t*>(result.data());
  std::copy(view.begin(), view.end(), u8Pointer);
  return result;
}
};
} // namespace CesiumJsonWriter