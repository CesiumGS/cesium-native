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
/**
 * @brief Wrapper around `rapidjson::Writer` for writing objects to JSON.
 */
class JsonWriter {
public:
  JsonWriter();
  virtual ~JsonWriter() = default;

  /**
   * @brief Writes a `null` value to the output.
   * @returns True if the write was successful.
   */
  virtual bool Null();
  /**
   * @brief Writes a boolean value to the output.
   * @param b The boolean value to write.
   * @returns True if the write was successful.
   */
  virtual bool Bool(bool b);
  /**
   * @brief Writes a signed integer value to the output.
   * @param i The integer value to write.
   * @returns True if the write was successful.
   */
  virtual bool Int(int i);
  /**
   * @brief Writes an unsigned integer value to the output.
   * @param i The integer value to write.
   * @returns True if the write was successful.
   */
  virtual bool Uint(unsigned int i);
  /**
   * @brief Writes an unsigned 64-bit integer value to the output.
   * @param i The integer value to write.
   * @returns True if the write was successful.
   */
  virtual bool Uint64(std::uint64_t i);
  /**
   * @brief Writes an signed 64-bit integer value to the output.
   * @param i The integer value to write.
   * @returns True if the write was successful.
   */
  virtual bool Int64(std::int64_t i);
  /**
   * @brief Writes a 64-bit floating point value to the output.
   * @param d The double value to write.
   * @returns True if the write was successful.
   */
  virtual bool Double(double d);
  /**
   * @brief Writes the given string as a number to the output without any kind
   * of special handling.
   *
   * @param str The raw number to write directly to the output.
   * @param length The length of the string.
   * @param copy If true, the string will be copied.
   * @returns True if the write was successful.
   */
  virtual bool RawNumber(const char* str, unsigned int length, bool copy);
  /**
   * @brief Writes the given string as an object key to the output.
   *
   * @param string The key to write.
   * @returns True if the write was successful.
   */
  virtual bool Key(std::string_view string);
  /**
   * @brief Writes the given string as a value to the output.
   *
   * @param string The string to write.
   * @returns True if the write was successful.
   */
  virtual bool String(std::string_view string);
  /**
   * @brief Writes the start of a JSON object to the output.
   *
   * @returns True if the write was successful.
   */
  virtual bool StartObject();
  /**
   * @brief Writes the end of a JSON object to the output.
   *
   * @returns True if the write was successful.
   */
  virtual bool EndObject();
  /**
   * @brief Writes the start of a JSON array to the output.
   *
   * @returns True if the write was successful.
   */
  virtual bool StartArray();
  /**
   * @brief Writes the end of a JSON array to the output.
   *
   * @returns True if the write was successful.
   */
  virtual bool EndArray();

  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Int.
   * @param value The int32_t value to write.
   */
  virtual void Primitive(std::int32_t value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Uint.
   * @param value The uint32_t value to write.
   */
  virtual void Primitive(std::uint32_t value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Int64.
   * @param value The int64_t value to write.
   */
  virtual void Primitive(std::int64_t value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Uint64.
   * @param value The uint64_t value to write.
   */
  virtual void Primitive(std::uint64_t value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Double.
   * @param value The float value to write.
   */
  virtual void Primitive(float value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Double.
   * @param value The double value to write.
   */
  virtual void Primitive(double value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref Null.
   * @param value The null value to write.
   */
  virtual void Primitive(std::nullptr_t value);
  /**
   * @brief Writes the given primitive to the output. This is a convenience
   * function for \ref String.
   * @param string The string value to write.
   */
  virtual void Primitive(std::string_view string);

  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Int.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::int32_t value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Uint.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::uint32_t value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Int64.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::int64_t value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Uint64.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::uint64_t value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref String.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::string_view value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Double.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, float value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Double.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, double value);
  /**
   * @brief Writes the given key and its corresponding value primitive to the
   * output. This is a convenience function for calling \ref Key followed by
   * \ref Null.
   * @param keyName The key to write to the output.
   * @param value The primitive value to write.
   */
  virtual void KeyPrimitive(std::string_view keyName, std::nullptr_t value);

  /**
   * @brief Writes an array to the output with the given key and calls the
   * provided callback to write values inside of the array. This is a
   * convenience function for calling \ref Key followed by \ref StartArray
   * followed by the values you wish to write and ending with \ref EndArray.
   *
   * @param keyName The key to write to the output.
   * @param insideArray The callback to run, after \ref StartArray but before
   * \ref EndArray.
   */
  virtual void KeyArray(
      std::string_view keyName,
      const std::function<void(void)>& insideArray);
  /**
   * @brief Writes an object to the output with the given key and calls the
   * provided callback to write values inside of the object. This is a
   * convenience function for calling \ref Key followed by \ref StartObject
   * followed by the values you wish to write and ending with \ref EndObject.
   *
   * @param keyName The key to write to the output.
   * @param insideObject The callback to run, after \ref StartObject but before
   * \ref EndObject.
   */
  virtual void KeyObject(
      std::string_view keyName,
      const std::function<void(void)>& insideObject);

  /**
   * @brief Obtains the written output as a string.
   */
  virtual std::string toString();
  /**
   * @brief Obtains the written output as a string_view.
   */
  virtual std::string_view toStringView();
  /**
   * @brief Obtains the written output as a buffer of bytes.
   */
  virtual std::vector<std::byte> toBytes();

  /**
   * @brief Emplaces a new error into the internal error buffer.
   */
  template <typename ErrorStr> void emplaceError(ErrorStr&& error) {
    _errors.emplace_back(std::forward<ErrorStr>(error));
  }

  /**
   * @brief Emplaces a new warning into the internal warning buffer.
   */
  template <typename WarningStr> void emplaceWarning(WarningStr&& warning) {
    _warnings.emplace_back(std::forward<WarningStr>(warning));
  }

  /**
   * @brief Obtains the current error buffer.
   */
  const std::vector<std::string>& getErrors() const { return _errors; }
  /**
   * @brief Obtains the current warning buffer.
   */
  const std::vector<std::string>& getWarnings() const { return _warnings; }

private:
  rapidjson::StringBuffer _compactBuffer;
  std::unique_ptr<rapidjson::Writer<rapidjson::StringBuffer>> _compact;

  std::vector<std::string> _errors;
  std::vector<std::string> _warnings;
};
} // namespace CesiumJsonWriter
