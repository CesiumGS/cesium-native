#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>

#include <rapidjson/document.h>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace rapidjson {
struct MemoryStream;
}

namespace CesiumJsonReader {

/**
 * @brief The result of {@link JsonReader::readJson}.
 */
template <typename T> struct ReadJsonResult {
  /**
   * @brief The value read from the JSON, or `std::nullopt` on error.
   */
  std::optional<T> value;

  /**
   * @brief Errors that occurred while reading.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings that occurred while reading.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Reads JSON.
 */
class CESIUMJSONREADER_API JsonReader {
public:
  /**
   * @brief Reads JSON from a byte buffer into a statically-typed class.
   *
   * @param data The buffer from which to read JSON.
   * @param handler The handler to receive the top-level JSON object. This
   * instance must:
   *   - Implement {@link IJsonHandler}.
   *   - Contain a `ValueType` type alias indicating the type of the instance to
   * be read into.
   *   - Have a `reset` method taking 1) a parent `IJsonHandler` pointer, and 2)
   * and a pointer to a value of type `ValueType`.
   * @return The result of reading the JSON.
   */
  template <typename T>
  static ReadJsonResult<typename T::ValueType>
  readJson(const std::span<const std::byte>& data, T& handler) {
    ReadJsonResult<typename T::ValueType> result;

    result.value.emplace();

    FinalJsonHandler finalHandler(result.warnings);
    handler.reset(&finalHandler, &result.value.value());

    JsonReader::internalRead(
        data,
        handler,
        finalHandler,
        result.errors,
        result.warnings);

    if (!result.errors.empty()) {
      result.value.reset();
    }

    return result;
  }

  /**
   * @brief Reads JSON from a `rapidjson::Value` into a statically-typed class.
   *
   * @param jsonValue The `rapidjson::Value` from which to read JSON.
   * @param handler The handler to receive the top-level JSON object. This
   * instance must:
   *   - Implement {@link IJsonHandler}.
   *   - Contain a `ValueType` type alias indicating the type of the instance to
   * be read into.
   *   - Have a `reset` method taking 1) a parent `IJsonHandler` pointer, and 2)
   * and a pointer to a value of type `ValueType`.
   * @return The result of reading the JSON.
   */
  template <typename T>
  static ReadJsonResult<typename T::ValueType>
  readJson(const rapidjson::Value& jsonValue, T& handler) {
    ReadJsonResult<typename T::ValueType> result;

    result.value.emplace();

    FinalJsonHandler finalHandler(result.warnings);
    handler.reset(&finalHandler, &result.value.value());

    JsonReader::internalRead(
        jsonValue,
        handler,
        finalHandler,
        result.errors,
        result.warnings);

    if (!result.errors.empty()) {
      result.value.reset();
    }

    return result;
  }

private:
  class FinalJsonHandler : public JsonHandler {
  public:
    FinalJsonHandler(std::vector<std::string>& warnings);
    virtual void reportWarning(
        const std::string& warning,
        std::vector<std::string>&& context) override;
    void setInputStream(rapidjson::MemoryStream* pInputStream) noexcept;

  private:
    std::vector<std::string>& _warnings;
    rapidjson::MemoryStream* _pInputStream;
  };

  static void internalRead(
      const std::span<const std::byte>& data,
      IJsonHandler& handler,
      FinalJsonHandler& finalHandler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);

  static void internalRead(
      const rapidjson::Value& jsonValue,
      IJsonHandler& handler,
      FinalJsonHandler& finalHandler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);
};

} // namespace CesiumJsonReader
