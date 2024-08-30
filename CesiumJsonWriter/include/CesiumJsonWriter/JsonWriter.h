#pragma once

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumJsonWriter {
class JsonWriter {
public:
  JsonWriter();
  virtual ~JsonWriter() {}

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

  template <typename ErrorStr> void emplaceError(ErrorStr&& error) {
    _errors.emplace_back(std::forward<ErrorStr>(error));
  }

  template <typename WarningStr> void emplaceWarning(WarningStr&& warning) {
    _warnings.emplace_back(std::forward<WarningStr>(warning));
  }

  const std::vector<std::string>& getErrors() const { return _errors; }
  const std::vector<std::string>& getWarnings() const { return _warnings; }

private:
  rapidjson::StringBuffer _compactBuffer;
  std::unique_ptr<rapidjson::Writer<rapidjson::StringBuffer>> _compact;

  std::vector<std::string> _errors;
  std::vector<std::string> _warnings;
};
} // namespace CesiumJsonWriter
