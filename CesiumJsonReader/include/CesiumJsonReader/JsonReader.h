#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <gsl/span>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace rapidjson {
struct MemoryStream;
}

namespace CesiumJsonReader {

/**
 * @brief The result of {@link Reader::readJson}.
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
   * @brief Reads JSON from a byte buffer.
   *
   * @param data The buffer from which to read JSON.
   * @param handler The handler to receive the top-level JSON object.
   * @return The result of reading the JSON.
   */
  template <typename T>
  static ReadJsonResult<typename T::ValueType>
  readJson(const gsl::span<const std::byte>& data, T& handler) {
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
      const gsl::span<const std::byte>& data,
      IJsonHandler& handler,
      FinalJsonHandler& finalHandler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);
};

} // namespace CesiumJsonReader
