#pragma once

#include <CesiumJsonReader/Library.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumJsonReader {
/**
 * @brief Base interface for all JSON handlers. Types that need to be
 * deserialized from JSON should implement `IJsonHandler` or a class that
 * derives from it. As the JSON is parsed, the corresponding `read...` method to
 * the token that was parsed will be called. These methods can return themselves
 * or a different handler to handle the value.
 */
class CESIUMJSONREADER_API IJsonHandler {
public:
  virtual ~IJsonHandler() = default;
  /**
   * @brief Called when the JSON parser encounters a `null`.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readNull() = 0;
  /**
   * @brief Called when the JSON parser encounters a boolean value.
   * @param b The boolean value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readBool(bool b) = 0;
  /**
   * @brief Called when the JSON parser encounters an int32 value.
   * @param i The int32 value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readInt32(int32_t i) = 0;
  /**
   * @brief Called when the JSON parser encounters a uint32 value.
   * @param i The uint32 value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readUint32(uint32_t i) = 0;
  /**
   * @brief Called when the JSON parser encounters an int64 value.
   * @param i The int64 value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readInt64(int64_t i) = 0;
  /**
   * @brief Called when the JSON parser encounters a uint64 value.
   * @param i The uint64 value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readUint64(uint64_t i) = 0;
  /**
   * @brief Called when the JSON parser encounters a double value.
   * @param d The double value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readDouble(double d) = 0;
  /**
   * @brief Called when the JSON parser encounters a string value.
   * @param str The string value.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readString(const std::string_view& str) = 0;

  /**
   * @brief Called when the JSON parser encounters the beginning of an object.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readObjectStart() = 0;
  /**
   * @brief Called when the JSON parser encounters a key while reading an
   * object.
   * @param str The key.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readObjectKey(const std::string_view& str) = 0;
  /**
   * @brief Called when the JSON parser encounters the end of an object.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readObjectEnd() = 0;

  /**
   * @brief Called when the JSON parser encounters the start of an array.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readArrayStart() = 0;
  /**
   * @brief Called when the JSON parser encounters the end of an array.
   * @returns A \ref IJsonHandler that will handle the next `read...` call.
   * This can be the same handler as the current one.
   */
  virtual IJsonHandler* readArrayEnd() = 0;

  /**
   * @brief Report a warning while reading JSON.
   *
   * @param warning The warning to report.
   * @param context Context information to include with this warning to help
   * debugging.
   */
  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) = 0;
};
} // namespace CesiumJsonReader
