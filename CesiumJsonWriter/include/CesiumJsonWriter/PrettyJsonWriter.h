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

class PrettyJsonWriter : public JsonWriter {
  rapidjson::StringBuffer _prettyBuffer;
  std::unique_ptr<rapidjson::PrettyWriter<rapidjson::StringBuffer>> pretty;

public:
  PrettyJsonWriter() noexcept;
  ~PrettyJsonWriter() {}

  // rapidjson methods
  bool Null() override;
  bool Bool(bool b) override;
  bool Int(int i) override;
  bool Uint(unsigned int i) override;
  bool Uint64(std::uint64_t i) override;
  bool Int64(std::int64_t i) override;
  bool Double(double d) override;
  bool RawNumber(const char* str, unsigned int length, bool copy) override;
  bool Key(std::string_view string) override;
  bool String(std::string_view string) override;
  bool StartObject() override;
  bool EndObject() override;
  bool StartArray() override;
  bool EndArray() override;

  // Primitive overloads
  void Primitive(std::int32_t value) override;
  void Primitive(std::uint32_t value) override;
  void Primitive(std::int64_t value) override;
  void Primitive(std::uint64_t value) override;
  void Primitive(float value) override;
  void Primitive(double value) override;
  void Primitive(std::nullptr_t value) override;
  void Primitive(std::string_view string) override;

  // Integral
  void KeyPrimitive(std::string_view keyName, std::int32_t value) override;
  void KeyPrimitive(std::string_view keyName, std::uint32_t value) override;
  void KeyPrimitive(std::string_view keyName, std::int64_t value) override;
  void KeyPrimitive(std::string_view keyName, std::uint64_t value) override;

  // String
  void KeyPrimitive(std::string_view keyName, std::string_view value) override;

  // Floating Point
  void KeyPrimitive(std::string_view keyName, float value) override;
  void KeyPrimitive(std::string_view keyName, double value) override;

  // Null
  void KeyPrimitive(std::string_view keyName, std::nullptr_t value) override;

  // Array / Objects
  void KeyArray(std::string_view keyName, std::function<void(void)> insideArray)
      override;

  void KeyObject(
      std::string_view keyName,
      std::function<void(void)> insideObject) override;

  std::string toString() override;
  std::string_view toStringView() override;
  std::vector<std::byte> toBytes() override;
};
} // namespace CesiumJsonWriter
